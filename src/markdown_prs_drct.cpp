/*
 * ==============================================================================
 * ASE CORE INFRASTRUCTURE IMPLEMENTATION
 * ==============================================================================
 *
 * @file        markdown_prs_drct.cpp
 * @brief       DirectiveParser - Two-phase directive parser for DSGN mode
 * @description Preprocesses raw text before cmark-gfm to replace block and
 *              leaf directives with HTML comment markers, then walks the AST
 *              to replace those markers with NODE_BLOCK_DIRECTIVE and
 *              NODE_LEAF_DIRECTIVE nodes. Also parses inline extensions.
 *
 * @module      ase-markdown
 * @layer       1 (Core)
 * @category    process/computation/algorithm
 *
 * @created     2026-04-12
 * @modified    2026-04-12
 * @version     00.00.01.00001 [seed]
 *
 * ==============================================================================
 * CORE INFRASTRUCTURE IMPLEMENTATION COMPLIANCE
 * ==============================================================================
 * [ ] NOT an ECS System implementation
 * [ ] Layer dependencies correct (L0: no ASE deps, L1: L0 only)
 * [ ] Own header included FIRST
 * [ ] No global mutable state
 * [ ] No static initialization order fiasco
 * [ ] Thread-safe implementations (pure or mutex-protected)
 * [ ] All error conditions handled
 * [ ] No exceptions thrown (use Result<T> pattern)
 * [ ] Implementation details in anonymous namespace
 * [ ] No inline implementations of template specializations here
 * [ ] Platform-specific code isolated and documented
 * [ ] Performance-critical code profiled and optimized
 * ==============================================================================
 */

#include <ase/markdown/markdown.hpp>
#include <ase/markdown/ast.hpp>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

namespace ase::markdown {

// Defined in markdown_prs_base.cpp — cmark-parses a raw fragment into an
// ase AST root so directive content (images, bold, code fences, …) becomes
// proper nodes instead of a single raw-text child.
Node* parse_fragment_to_ase(Document& doc, const char* content, uint32_t content_len);

namespace {

// Move every child from src to dst. Used to graft a parsed fragment's
// top-level nodes into a directive node without keeping the wrapper
// NODE_DOCUMENT around.
void move_children(Node* dst, Node* src) {
    if (!dst || !src) return;
    Node* c = src->first_child;
    while (c != nullptr) {
        Node* next = c->next_sibling;
        c->parent = nullptr;
        c->next_sibling = nullptr;
        append_child(dst, c);
        c = next;
    }
    src->first_child = nullptr;
}

// Attach the parsed-markdown children of `content` as children of
// `parent`. Calls the cmark fragment parser in base.cpp and grafts the
// resulting document root's children into the parent.
void attach_markdown_fragment(Document& doc, Node* parent,
                              const char* content, uint32_t content_len) {
    if (!parent || !content || content_len == 0) return;
    Node* frag = parse_fragment_to_ase(doc, content, content_len);
    if (!frag) return;
    move_children(parent, frag);
}

// ── Attribute Parser ───────────────────────────────────────────────

uint16_t parse_attrs(const char* attr_str, uint32_t attr_len, Document& doc, Attr*& out) {
    if (!attr_str || attr_len == 0) { out = nullptr; return 0; }

    uint16_t max_count = 1;
    for (uint32_t i = 0; i < attr_len; i++) {
        if (attr_str[i] == '=' || attr_str[i] == ' ') max_count++;
    }

    out = static_cast<Attr*>(doc.arena->allocate(max_count * sizeof(Attr)));
    uint16_t actual = 0;
    uint32_t pos = 0;

    while (pos < attr_len && actual < max_count) {
        while (pos < attr_len && (attr_str[pos] == ' ' || attr_str[pos] == '\t')) pos++;
        if (pos >= attr_len) break;

        uint32_t key_start = pos;
        while (pos < attr_len && attr_str[pos] != '=' && attr_str[pos] != ' ' && attr_str[pos] != '\t') pos++;
        uint32_t key_len = pos - key_start;
        if (key_len == 0) break;

        char* key = alloc_string(doc, attr_str + key_start, key_len);

        if (pos < attr_len && attr_str[pos] == '=') {
            pos++;
            char* val = nullptr;
            if (pos < attr_len && (attr_str[pos] == '"' || attr_str[pos] == '\'')) {
                char quote = attr_str[pos];
                pos++;
                uint32_t val_start = pos;
                while (pos < attr_len && attr_str[pos] != quote) pos++;
                val = alloc_string(doc, attr_str + val_start, pos - val_start);
                if (pos < attr_len) pos++;
            } else {
                uint32_t val_start = pos;
                while (pos < attr_len && attr_str[pos] != ' ' && attr_str[pos] != '\t') pos++;
                val = alloc_string(doc, attr_str + val_start, pos - val_start);
            }
            out[actual] = { key, val };
        } else {
            out[actual] = { key, key };
        }
        actual++;
    }

    return actual;
}

// ── Stored Directive Data (populated during preprocess) ────────────

struct DirectiveData {
    std::string name;
    std::string attrs_str;
    std::string content;
    bool is_block;
};

static std::vector<DirectiveData> g_directives;

// ── Line scanning helpers ──────────────────────────────────────────

bool is_block_open(const char* line, uint32_t len,
                   uint32_t& name_start, uint32_t& name_end,
                   uint32_t& attr_start, uint32_t& attr_end) {
    if (len < 4 || line[0] != ':' || line[1] != ':' || line[2] != ':') return false;
    if (len == 3 || (len == 4 && line[3] == '\n')) return false;

    uint32_t p = 3;
    if (p < len && (line[p] == ' ' || line[p] == '\n')) return false;

    name_start = p;
    while (p < len && line[p] != '{' && line[p] != '\n' && line[p] != ' ' && line[p] != '\r') p++;
    name_end = p;
    if (name_end == name_start) return false;

    attr_start = 0;
    attr_end = 0;
    if (p < len && line[p] == '{') {
        attr_start = p + 1;
        while (p < len && line[p] != '}') p++;
        attr_end = p;
        if (p < len) p++;
    }

    return true;
}

bool is_block_close(const char* line, uint32_t len) {
    if (len < 3) return false;
    if (line[0] != ':' || line[1] != ':' || line[2] != ':') return false;
    for (uint32_t i = 3; i < len; i++) {
        if (line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\r') return false;
    }
    return true;
}

bool is_leaf_directive(const char* line, uint32_t len,
                       uint32_t& name_start, uint32_t& name_end,
                       uint32_t& attr_start, uint32_t& attr_end) {
    if (len < 3 || line[0] != ':' || line[1] != ':') return false;
    if (len > 2 && line[2] == ':') return false;

    uint32_t p = 2;
    if (p >= len || line[p] == ' ' || line[p] == '\n') return false;

    name_start = p;
    while (p < len && line[p] != '{' && line[p] != '\n' && line[p] != ' ' && line[p] != '\r') p++;
    name_end = p;
    if (name_end == name_start) return false;

    attr_start = 0;
    attr_end = 0;
    if (p < len && line[p] == '{') {
        attr_start = p + 1;
        while (p < len && line[p] != '}') p++;
        attr_end = p;
    }

    return true;
}

// ── Build directive nodes from stored data ─────────────────────────

void parse_children(Document& doc, Node* parent, const char* content, uint32_t content_len) {
    uint32_t pos = 0;

    while (pos < content_len) {
        uint32_t line_start = pos;
        uint32_t line_end = pos;
        while (line_end < content_len && content[line_end] != '\n') line_end++;

        const char* line = content + line_start;
        uint32_t line_len = line_end - line_start;

        uint32_t ns, ne, as, ae;
        if (is_leaf_directive(line, line_len, ns, ne, as, ae)) {
            Node* child = alloc_node(doc, NODE_LEAF_DIRECTIVE);
            child->directive_name = alloc_string(doc, line + ns, ne - ns);

            if (as > 0 && ae > as) {
                child->attr_count = parse_attrs(line + as, ae - as, doc, child->attrs);
            }

            uint32_t content_start = (ae > 0) ? ae + 1 : ne;
            while (content_start < line_len && (line[content_start] == ' ' || line[content_start] == '\t')) content_start++;

            std::string child_text;
            if (content_start < line_len) {
                child_text.append(line + content_start, line_len - content_start);
            }

            uint32_t next_pos = line_end + 1;
            while (next_pos < content_len) {
                uint32_t next_line_start = next_pos;
                uint32_t next_line_end = next_pos;
                while (next_line_end < content_len && content[next_line_end] != '\n') next_line_end++;

                const char* next_line = content + next_line_start;
                uint32_t next_len = next_line_end - next_line_start;

                uint32_t dummy_ns, dummy_ne, dummy_as, dummy_ae;
                if (next_len > 0 && is_leaf_directive(next_line, next_len, dummy_ns, dummy_ne, dummy_as, dummy_ae)) break;
                bool is_blank = true;
                for (uint32_t i = 0; i < next_len; i++) {
                    if (next_line[i] != ' ' && next_line[i] != '\t' && next_line[i] != '\r') { is_blank = false; break; }
                }
                if (is_blank) {
                    uint32_t peek = next_line_end + 1;
                    if (peek < content_len && content[peek] == ':' && peek + 1 < content_len && content[peek + 1] == ':') break;
                }

                if (!child_text.empty()) child_text += '\n';
                child_text.append(next_line, next_len);
                next_pos = next_line_end + 1;
            }

            if (!child_text.empty()) {
                // Cmark-parse the leaf directive's multi-line content so
                // embedded images / code / emphasis become proper AST
                // nodes instead of a single raw-text child.
                attach_markdown_fragment(doc, child,
                                         child_text.c_str(),
                                         static_cast<uint32_t>(child_text.size()));
            }

            append_child(parent, child);
            pos = (line_end < content_len) ? line_end + 1 : content_len;
            uint32_t skip_pos = line_end + 1;
            while (skip_pos < content_len) {
                uint32_t sl = skip_pos;
                while (sl < content_len && content[sl] != '\n') sl++;
                const char* sl_line = content + skip_pos;
                uint32_t sl_len = sl - skip_pos;
                uint32_t d_ns, d_ne, d_as, d_ae;
                if (sl_len > 0 && is_leaf_directive(sl_line, sl_len, d_ns, d_ne, d_as, d_ae)) break;
                skip_pos = sl + 1;
            }
            pos = skip_pos;
        } else {
            // Pre-leaf content: accumulate every non-directive line until
            // we hit a leaf directive or run out of content, then
            // cmark-parse the whole block so images / code / emphasis
            // become proper nodes. Without this, block directives with
            // plain markdown content (e.g. :::figure\n![alt](url)\n:::)
            // lose their image to a raw-text child.
            if (!parent->first_child) {
                uint32_t frag_start = line_start;
                uint32_t frag_end = line_end;
                uint32_t scan = line_end + 1;
                while (scan < content_len) {
                    uint32_t sl_start = scan;
                    uint32_t sl_end = scan;
                    while (sl_end < content_len && content[sl_end] != '\n') sl_end++;
                    uint32_t d_ns, d_ne, d_as, d_ae;
                    if (is_leaf_directive(content + sl_start, sl_end - sl_start,
                                          d_ns, d_ne, d_as, d_ae)) {
                        break;
                    }
                    frag_end = sl_end;
                    scan = sl_end + 1;
                }
                if (frag_end > frag_start) {
                    attach_markdown_fragment(doc, parent,
                                             content + frag_start,
                                             frag_end - frag_start);
                }
                pos = (scan < content_len) ? scan : content_len;
                continue;
            }
            pos = (line_end < content_len) ? line_end + 1 : content_len;
        }
    }
}

Node* build_directive_node(Document& doc, const DirectiveData& dd) {
    Node* node = alloc_node(doc, dd.is_block ? NODE_BLOCK_DIRECTIVE : NODE_LEAF_DIRECTIVE);
    node->directive_name = alloc_string(doc, dd.name.c_str(), static_cast<uint32_t>(dd.name.size()));

    if (!dd.attrs_str.empty()) {
        node->attr_count = parse_attrs(dd.attrs_str.c_str(), static_cast<uint32_t>(dd.attrs_str.size()), doc, node->attrs);
    }

    if (dd.is_block && !dd.content.empty()) {
        parse_children(doc, node, dd.content.c_str(), static_cast<uint32_t>(dd.content.size()));

        if (!node->first_child) {
            Node* text_node = alloc_node(doc, NODE_TEXT);
            text_node->text = alloc_string(doc, dd.content.c_str(), static_cast<uint32_t>(dd.content.size()));
            text_node->text_len = static_cast<uint32_t>(dd.content.size());
            append_child(node, text_node);
        }
    }

    return node;
}

// ── AST walker: replace HTML_BLOCK markers with directive nodes ────

void replace_markers_in_children(Document& doc, Node* parent) {
    if (!parent) return;

    Node* prev = nullptr;
    Node* child = parent->first_child;

    while (child) {
        Node* next = child->next_sibling;

        if (child->type == NODE_HTML_BLOCK && child->text && child->text_len > 20) {
            const char* t = child->text;
            bool is_block_marker = (std::strncmp(t, "<!-- ASE_BLOCK_DIR_", 19) == 0);
            bool is_leaf_marker = (std::strncmp(t, "<!-- ASE_LEAF_DIR_", 18) == 0);

            if (is_block_marker || is_leaf_marker) {
                const char* num_start = t + (is_block_marker ? 19 : 18);
                int idx = 0;
                while (*num_start >= '0' && *num_start <= '9') {
                    idx = idx * 10 + (*num_start - '0');
                    num_start++;
                }

                if (idx >= 0 && idx < static_cast<int>(g_directives.size())) {
                    Node* dir_node = build_directive_node(doc, g_directives[idx]);

                    dir_node->next_sibling = next;
                    dir_node->parent = parent;

                    if (prev) {
                        prev->next_sibling = dir_node;
                    } else {
                        parent->first_child = dir_node;
                    }

                    prev = dir_node;
                    child = next;
                    continue;
                }
            }
        }

        replace_markers_in_children(doc, child);

        prev = child;
        child = next;
    }
}

}  // anonymous namespace

// ── Phase 1: Pre-process raw text before cmark-gfm ────────────────

std::string preprocess_directives(const char* input, uint32_t len) {
    g_directives.clear();

    std::string result;
    result.reserve(len);

    uint32_t pos = 0;
    while (pos < len) {
        uint32_t line_start = pos;
        uint32_t line_end = pos;
        while (line_end < len && input[line_end] != '\n') line_end++;

        const char* line = input + line_start;
        uint32_t line_len = line_end - line_start;

        uint32_t ns, ne, as, ae;

        if (is_block_open(line, line_len, ns, ne, as, ae)) {
            DirectiveData dd;
            dd.name.assign(line + ns, ne - ns);
            if (as > 0 && ae > as) dd.attrs_str.assign(line + as, ae - as);
            dd.is_block = true;

            uint32_t content_start = line_end + 1;
            uint32_t scan = content_start;
            uint32_t content_end = len;
            bool found_close = false;

            while (scan < len) {
                uint32_t cl_start = scan;
                uint32_t cl_end = scan;
                while (cl_end < len && input[cl_end] != '\n') cl_end++;

                if (is_block_close(input + cl_start, cl_end - cl_start)) {
                    content_end = cl_start;
                    found_close = true;
                    pos = cl_end + 1;
                    break;
                }
                scan = cl_end + 1;
            }

            if (!found_close) {
                content_end = len;
                pos = len;
            }

            if (content_end > content_start) {
                dd.content.assign(input + content_start, content_end - content_start);
                while (!dd.content.empty() && (dd.content.back() == '\n' || dd.content.back() == '\r')) {
                    dd.content.pop_back();
                }
            }

            int idx = static_cast<int>(g_directives.size());
            g_directives.push_back(std::move(dd));

            result += "\n<!-- ASE_BLOCK_DIR_";
            result += std::to_string(idx);
            result += " -->\n\n";
            continue;
        }

        if (is_leaf_directive(line, line_len, ns, ne, as, ae)) {
            // The line may carry several leaf directives back-to-back
            // (e.g. ``::badge{…} ::badge{…} ::badge{…}``). Walk the
            // entire line and emit one HTML marker per directive
            // encountered. Anything between directives that is not
            // itself a leaf directive becomes inline content of the
            // most recently emitted directive.
            uint32_t cursor = 0;
            while (cursor < line_len) {
                uint32_t cns = 0, cne = 0, cas = 0, cae = 0;
                if (cursor + 2 >= line_len ||
                    !is_leaf_directive(line + cursor, line_len - cursor,
                                       cns, cne, cas, cae)) {
                    // No more directives on this line; the trailing
                    // text becomes content of the previously-emitted
                    // directive (if any).
                    if (!g_directives.empty()) {
                        uint32_t tail_start = cursor;
                        while (tail_start < line_len &&
                               (line[tail_start] == ' ' || line[tail_start] == '\t')) {
                            ++tail_start;
                        }
                        if (tail_start < line_len) {
                            auto& last = g_directives.back();
                            if (!last.content.empty()) last.content.push_back(' ');
                            last.content.append(line + tail_start, line_len - tail_start);
                        }
                    }
                    break;
                }

                DirectiveData dd;
                dd.name.assign(line + cursor + cns, cne - cns);
                if (cas > 0 && cae > cas) {
                    dd.attrs_str.assign(line + cursor + cas, cae - cas);
                }
                dd.is_block = false;

                int idx = static_cast<int>(g_directives.size());
                g_directives.push_back(std::move(dd));

                result += "\n<!-- ASE_LEAF_DIR_";
                result += std::to_string(idx);
                result += " -->\n\n";

                // Advance past the directive token (name + optional
                // {attrs}) and any trailing whitespace.
                uint32_t advance = (cae > 0) ? cae + 1 : cne;
                cursor += advance;
                while (cursor < line_len &&
                       (line[cursor] == ' ' || line[cursor] == '\t')) {
                    ++cursor;
                }
            }

            pos = line_end + 1;
            continue;
        }

        result.append(input + line_start, line_len);
        result += '\n';
        pos = line_end + 1;
    }

    return result;
}

// ── Phase 2: Replace HTML markers with directive nodes ─────────────

void pass_directives(Document& doc) {
    if (g_directives.empty()) return;
    replace_markers_in_children(doc, doc.root);
    g_directives.clear();
}

// ── Phase 2: Inline Extensions in Text Nodes ──────────────────────

namespace {

void process_inline_extensions(Node* node, Document& doc) {
    if (!node) return;

    Node* child = node->first_child;
    while (child) {
        Node* next = child->next_sibling;
        process_inline_extensions(child, doc);
        child = next;
    }

    if (node->type != NODE_TEXT || !node->text || node->text_len == 0) return;

    const char* text = node->text;
    uint32_t len = node->text_len;
    uint32_t pos = 0;
    uint32_t last_end = 0;

    Node* first_new = nullptr;
    Node* last_new = nullptr;

    auto append_node = [&](Node* n) {
        if (!first_new) first_new = n;
        else last_new->next_sibling = n;
        last_new = n;
    };

    auto emit_text_before = [&](uint32_t end) {
        if (end > last_end) {
            auto* txt = alloc_node(doc, NODE_TEXT);
            txt->text = alloc_string(doc, text + last_end, end - last_end);
            txt->text_len = end - last_end;
            append_node(txt);
        }
    };

    while (pos < len) {
        // [[wiki link]]
        if (pos + 3 < len && text[pos] == '[' && text[pos + 1] == '[') {
            uint32_t start = pos + 2;
            uint32_t end = start;
            while (end + 1 < len && !(text[end] == ']' && text[end + 1] == ']')) end++;
            if (end + 1 < len) {
                emit_text_before(pos);
                auto* wiki = alloc_node(doc, NODE_WIKI_LINK);
                wiki->text = alloc_string(doc, text + start, end - start);
                wiki->text_len = end - start;
                append_node(wiki);
                last_end = end + 2;
                pos = last_end;
                continue;
            }
        }

        // {{glossary term}}
        if (pos + 3 < len && text[pos] == '{' && text[pos + 1] == '{') {
            uint32_t start = pos + 2;
            uint32_t end = start;
            while (end + 1 < len && !(text[end] == '}' && text[end + 1] == '}')) end++;
            if (end + 1 < len) {
                emit_text_before(pos);
                auto* gloss = alloc_node(doc, NODE_GLOSSARY_TERM);
                gloss->text = alloc_string(doc, text + start, end - start);
                gloss->text_len = end - start;
                append_node(gloss);
                last_end = end + 2;
                pos = last_end;
                continue;
            }
        }

        // {ref:path#anchor|display}
        if (pos + 5 < len && text[pos] == '{' && text[pos + 1] == 'r' &&
            text[pos + 2] == 'e' && text[pos + 3] == 'f' && text[pos + 4] == ':') {
            uint32_t start = pos + 5;
            uint32_t end = start;
            while (end < len && text[end] != '}') end++;
            if (end < len) {
                emit_text_before(pos);
                auto* xref = alloc_node(doc, NODE_CROSS_REF);
                uint32_t pipe = start;
                while (pipe < end && text[pipe] != '|') pipe++;
                xref->url = alloc_string(doc, text + start, pipe - start);
                if (pipe < end) {
                    xref->text = alloc_string(doc, text + pipe + 1, end - pipe - 1);
                    xref->text_len = end - pipe - 1;
                } else {
                    xref->text = xref->url;
                    xref->text_len = pipe - start;
                }
                append_node(xref);
                last_end = end + 1;
                pos = last_end;
                continue;
            }
        }

        // {version}
        if (pos + 8 < len && text[pos] == '{' && text[pos + 1] == 'v' &&
            text[pos + 2] == 'e' && text[pos + 3] == 'r' && text[pos + 4] == 's' &&
            text[pos + 5] == 'i' && text[pos + 6] == 'o' && text[pos + 7] == 'n' &&
            text[pos + 8] == '}') {
            emit_text_before(pos);
            auto* ver = alloc_node(doc, NODE_VERSION_INSERT);
            append_node(ver);
            last_end = pos + 9;
            pos = last_end;
            continue;
        }

        // :tip[visible text]{tooltip content}
        if (pos + 4 < len && text[pos] == ':' && text[pos + 1] == 't' &&
            text[pos + 2] == 'i' && text[pos + 3] == 'p' && text[pos + 4] == '[') {
            uint32_t bracket_start = pos + 5;
            uint32_t bracket_end = bracket_start;
            while (bracket_end < len && text[bracket_end] != ']') bracket_end++;
            if (bracket_end < len && bracket_end + 1 < len && text[bracket_end + 1] == '{') {
                uint32_t brace_start = bracket_end + 2;
                uint32_t brace_end = brace_start;
                while (brace_end < len && text[brace_end] != '}') brace_end++;
                if (brace_end < len) {
                    emit_text_before(pos);
                    auto* tip = alloc_node(doc, NODE_TEXT_DIRECTIVE);
                    tip->directive_name = alloc_string(doc, "tip", 3);
                    tip->text = alloc_string(doc, text + bracket_start, bracket_end - bracket_start);
                    tip->text_len = bracket_end - bracket_start;
                    tip->title = alloc_string(doc, text + brace_start, brace_end - brace_start);
                    append_node(tip);
                    last_end = brace_end + 1;
                    pos = last_end;
                    continue;
                }
            }
        }

        pos++;
    }

    if (!first_new) return;

    if (last_end < len) {
        auto* txt = alloc_node(doc, NODE_TEXT);
        txt->text = alloc_string(doc, text + last_end, len - last_end);
        txt->text_len = len - last_end;
        append_node(txt);
    }

    node->type = first_new->type;
    node->text = first_new->text;
    node->text_len = first_new->text_len;
    node->directive_name = first_new->directive_name;
    node->url = first_new->url;
    node->title = first_new->title;

    Node* tail = first_new->next_sibling;
    if (tail) {
        Node* orig_next = node->next_sibling;
        node->next_sibling = tail;
        Node* chain_end = tail;
        while (chain_end->next_sibling) chain_end = chain_end->next_sibling;
        chain_end->next_sibling = orig_next;
        Node* n = tail;
        while (n && n != orig_next) { n->parent = node->parent; n = n->next_sibling; }
    }
}

}  // anonymous namespace

void pass_inline_extensions(Document& doc) {
    process_inline_extensions(doc.root, doc);
}

}  // namespace ase::markdown
