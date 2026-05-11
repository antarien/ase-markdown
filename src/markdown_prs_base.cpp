/*
 * ==============================================================================
 * ASE CORE INFRASTRUCTURE IMPLEMENTATION
 * ==============================================================================
 *
 * @file        markdown_prs_base.cpp
 * @brief       Markdown parser entry point — cmark-gfm bridge + AST conversion
 * @description Owns the public parse() / free_document() implementation. Wraps
 *              cmark-gfm with the GFM extension set (table, autolink, strike,
 *              tasklist), converts cmark's internal node tree to the
 *              ase::markdown POD AST, and runs the post-processing passes
 *              (callouts, math, icons, directives, inline extensions). The
 *              cmark-gfm short-name macros are disabled at build time via the
 *              CMARK_NO_SHORT_NAMES compile definition (see CMakeLists.txt) so
 *              they cannot collide with the ASE constexpr NODE_* identifiers.
 *
 * @module      ase-markdown
 * @layer       1 (Core)
 * @category    process/computation/algorithm
 *
 * @created     2026-04-12
 * @modified    2026-04-13
 * @version     00.00.02.00002 [poc]
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
#include <ase/markdown/frontmatter.hpp>

// CMARK_NO_SHORT_NAMES is injected by CMakeLists.txt as a compile definition.
// With it set, cmark-gfm.h does NOT define the NODE_DOCUMENT/NODE_HEADING/...
// short-name macros, so the ASE constexpr identifiers in types.hpp remain
// intact and correct AST node types are produced.
#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>
#include <table.h>
#include <strikethrough.h>

#include <memory>
#include <string>

namespace ase::markdown {

namespace {

// ── Local string helpers (no <cstring>) ─────────────────────────────

uint32_t cstr_len(const char* s) {
    if (s == nullptr) return 0;
    uint32_t n = 0;
    while (s[n] != '\0') ++n;
    return n;
}

bool streq(const char* a, const char* b) {
    if (a == nullptr || b == nullptr) return false;
    uint32_t i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return false;
        ++i;
    }
    return a[i] == b[i];
}

// ── cmark node-type → ASE node-type (no switch/case) ────────────────

uint8_t map_node_type(cmark_node_type ctype) {
    if (ctype == CMARK_NODE_DOCUMENT)       return NODE_DOCUMENT;
    if (ctype == CMARK_NODE_HEADING)        return NODE_HEADING;
    if (ctype == CMARK_NODE_PARAGRAPH)      return NODE_PARAGRAPH;
    if (ctype == CMARK_NODE_TEXT)           return NODE_TEXT;
    if (ctype == CMARK_NODE_SOFTBREAK)      return NODE_SOFTBREAK;
    if (ctype == CMARK_NODE_LINEBREAK)      return NODE_LINEBREAK;
    if (ctype == CMARK_NODE_CODE_BLOCK)     return NODE_CODE_BLOCK;
    if (ctype == CMARK_NODE_CODE)           return NODE_INLINE_CODE;
    if (ctype == CMARK_NODE_BLOCK_QUOTE)    return NODE_BLOCKQUOTE;
    if (ctype == CMARK_NODE_LIST)           return NODE_LIST;
    if (ctype == CMARK_NODE_ITEM)           return NODE_LIST_ITEM;
    if (ctype == CMARK_NODE_THEMATIC_BREAK) return NODE_THEMATIC_BREAK;
    if (ctype == CMARK_NODE_LINK)           return NODE_LINK;
    if (ctype == CMARK_NODE_IMAGE)          return NODE_IMAGE;
    if (ctype == CMARK_NODE_EMPH)           return NODE_EMPHASIS;
    if (ctype == CMARK_NODE_STRONG)         return NODE_STRONG;
    if (ctype == CMARK_NODE_HTML_BLOCK)     return NODE_HTML_BLOCK;
    if (ctype == CMARK_NODE_HTML_INLINE)    return NODE_HTML_INLINE;
    return NODE_TEXT;
}

// ── Recursive cmark → ase::markdown tree conversion ─────────────────

Node* convert_node(Document& doc, cmark_node* cnode) {
    if (cnode == nullptr) return nullptr;

    cmark_node_type raw_type = cmark_node_get_type(cnode);
    uint8_t type = map_node_type(raw_type);

    // Code blocks: classify by fence info language tag.
    if (type == NODE_CODE_BLOCK) {
        const char* fence_info = cmark_node_get_fence_info(cnode);
        if (fence_info != nullptr) {
            if      (streq(fence_info, "mermaid"))  type = NODE_MERMAID_BLOCK;
            else if (streq(fence_info, "diff"))     type = NODE_DIFF_BLOCK;
            else if (streq(fence_info, "svgbob"))   type = NODE_SVGBOB_BLOCK;
            else if (streq(fence_info, "ase-math")) type = NODE_ASEMATH_BLOCK;
            else if (streq(fence_info, "plantuml")) type = NODE_PLANTUML_BLOCK;
            else if (streq(fence_info, "puml"))     type = NODE_PLANTUML_BLOCK;
        }
    }

    // GFM table extension nodes are reported via the extension API,
    // not the core enum — map them by their extension-defined type code.
    if (raw_type == CMARK_NODE_TABLE)      type = NODE_TABLE;
    if (raw_type == CMARK_NODE_TABLE_ROW)  type = NODE_TABLE_ROW;
    if (raw_type == CMARK_NODE_TABLE_CELL) type = NODE_TABLE_CELL;

    // GFM strikethrough extension node.
    if (raw_type == CMARK_NODE_STRIKETHROUGH) type = NODE_STRIKETHROUGH;

    Node* node = alloc_node(doc, type);
    if (node == nullptr) return nullptr;

    // Literal text payload (inline code, code blocks, text nodes, …).
    const char* literal = cmark_node_get_literal(cnode);
    if (literal != nullptr) {
        uint32_t len = cstr_len(literal);
        node->text = alloc_string(doc, literal, len);
        node->text_len = len;
    }

    if (type == NODE_HEADING) {
        node->heading_level = static_cast<uint8_t>(cmark_node_get_heading_level(cnode));
    }

    if (type == NODE_LIST) {
        node->list_ordered = (cmark_node_get_list_type(cnode) == CMARK_ORDERED_LIST) ? 1 : 0;
        node->list_start = static_cast<uint32_t>(cmark_node_get_list_start(cnode));
    }

    if (type == NODE_CODE_BLOCK || type == NODE_MERMAID_BLOCK ||
        type == NODE_DIFF_BLOCK || type == NODE_SVGBOB_BLOCK   ||
        type == NODE_ASEMATH_BLOCK) {
        const char* info = cmark_node_get_fence_info(cnode);
        if (info != nullptr && info[0] != '\0') {
            node->language = alloc_string(doc, info, cstr_len(info));
        }
    }

    if (type == NODE_LINK || type == NODE_IMAGE) {
        const char* url = cmark_node_get_url(cnode);
        if (url != nullptr) {
            node->url = alloc_string(doc, url, cstr_len(url));
        }
        const char* title = cmark_node_get_title(cnode);
        if (title != nullptr && title[0] != '\0') {
            node->title = alloc_string(doc, title, cstr_len(title));
        }
    }

    cmark_node* child = cmark_node_first_child(cnode);
    while (child != nullptr) {
        Node* child_node = convert_node(doc, child);
        if (child_node != nullptr) {
            append_child(node, child_node);
        }
        child = cmark_node_next(child);
    }

    return node;
}

}  // namespace

Document parse(const char* input, uint32_t len, ParseOptions opts) {
    // Document holds raw buffer / arena pointers (POD-friendly). Ownership is
    // transferred from local std::unique_ptr instances via release(), so no
    // raw new/delete or malloc/free appears in this translation unit. The
    // pointers are reclaimed in free_document() through unique_ptr re-adoption.
    auto buffer_owner = std::make_unique<char[]>(DOCUMENT_ARENA_SIZE);
    auto arena_owner  = std::make_unique<ase::alloc::Arena>(buffer_owner.get(),
                                                            DOCUMENT_ARENA_SIZE);

    Document doc{};
    doc.buffer_size = DOCUMENT_ARENA_SIZE;
    doc.buffer = buffer_owner.release();
    doc.arena  = arena_owner.release();

    // Strip frontmatter before the cmark parse so it does not appear as a
    // paragraph at the top of the document tree.
    const char* body = input;
    uint32_t body_len = len;
    if (opts.parse_frontmatter) {
        uint32_t offset = parse_frontmatter(input, len, doc.frontmatter, doc);
        if (offset > 0) {
            body = input + offset;
            body_len = len - offset;
        }
    }

    // Directive preprocessing runs for BOTH modes. The TECH/DSGN split
    // was only ever UI classification (which catalogue a doc belongs to),
    // never a parser-level separation. Every `:::block{attrs}…:::` and
    // `::leaf{attrs}` in the raw input is replaced with an HTML comment
    // marker so it survives cmark-gfm as an HTML_BLOCK, then restored to
    // NODE_BLOCK_DIRECTIVE / NODE_LEAF_DIRECTIVE by pass_directives().
    // Documents without any `:::` lines pay zero cost (the scanner is a
    // single forward pass and simply emits the input unchanged).
    std::string preprocessed = preprocess_directives(body, body_len);
    body = preprocessed.c_str();
    body_len = static_cast<uint32_t>(preprocessed.size());

    cmark_gfm_core_extensions_ensure_registered();

    cmark_parser* parser = cmark_parser_new(CMARK_OPT_DEFAULT);
    cmark_syntax_extension* table_ext    = cmark_find_syntax_extension("table");
    cmark_syntax_extension* autolink_ext = cmark_find_syntax_extension("autolink");
    cmark_syntax_extension* strike_ext   = cmark_find_syntax_extension("strikethrough");
    cmark_syntax_extension* tasklist_ext = cmark_find_syntax_extension("tasklist");

    if (table_ext    != nullptr) cmark_parser_attach_syntax_extension(parser, table_ext);
    if (autolink_ext != nullptr) cmark_parser_attach_syntax_extension(parser, autolink_ext);
    if (strike_ext   != nullptr) cmark_parser_attach_syntax_extension(parser, strike_ext);
    if (tasklist_ext != nullptr) cmark_parser_attach_syntax_extension(parser, tasklist_ext);

    cmark_parser_feed(parser, body, static_cast<size_t>(body_len));
    cmark_node* cmark_root = cmark_parser_finish(parser);

    doc.root = convert_node(doc, cmark_root);

    cmark_node_free(cmark_root);
    cmark_parser_free(parser);

    // Post-processing passes — both parse modes.
    pass_callouts(doc);
    pass_math(doc);
    pass_icons(doc);

    // Directive passes — run regardless of mode so TECH docs that happen
    // to use `:::` blocks or `::name` leaves render identically to DSGN
    // docs. Doing both passes unconditionally removes the parser-level
    // TECH/DSGN fork and leaves mode as a pure UI classification.
    pass_directives(doc);
    pass_inline_extensions(doc);

    return doc;
}

void free_document(Document& doc) {
    // Re-adopt the raw pointers into std::unique_ptr instances so destruction
    // happens through the standard library deleter — no raw delete in
    // application code.
    if (doc.arena != nullptr) {
        std::unique_ptr<ase::alloc::Arena> arena_reclaim{doc.arena};
        doc.arena = nullptr;
    }
    if (doc.buffer != nullptr) {
        std::unique_ptr<char[]> buffer_reclaim{doc.buffer};
        doc.buffer = nullptr;
    }
    doc.buffer_size = 0;
    doc.root = nullptr;
}

// ── Parse a raw markdown fragment into ase AST children ──────────────
//
// Used by markdown_prs_drct.cpp to cmark-parse the content of block and
// leaf directives so that `![img](url)`, `**bold**`, fenced code blocks
// and other markdown inside a directive becomes proper AST nodes instead
// of a single raw-text child. Does NOT run the preprocess/pass pipeline
// (directive preprocessing is handled by the outer parse() call; nested
// block directives inside a directive's content are not supported).
// Returns the top-level ase node that wraps the cmark document tree —
// the caller transfers its children to the directive node.
Node* parse_fragment_to_ase(Document& doc, const char* content, uint32_t content_len) {
    if (content == nullptr || content_len == 0) return nullptr;
    cmark_gfm_core_extensions_ensure_registered();
    cmark_parser* parser = cmark_parser_new(CMARK_OPT_DEFAULT);
    cmark_syntax_extension* table_ext    = cmark_find_syntax_extension("table");
    cmark_syntax_extension* autolink_ext = cmark_find_syntax_extension("autolink");
    cmark_syntax_extension* strike_ext   = cmark_find_syntax_extension("strikethrough");
    cmark_syntax_extension* tasklist_ext = cmark_find_syntax_extension("tasklist");
    if (table_ext    != nullptr) cmark_parser_attach_syntax_extension(parser, table_ext);
    if (autolink_ext != nullptr) cmark_parser_attach_syntax_extension(parser, autolink_ext);
    if (strike_ext   != nullptr) cmark_parser_attach_syntax_extension(parser, strike_ext);
    if (tasklist_ext != nullptr) cmark_parser_attach_syntax_extension(parser, tasklist_ext);
    cmark_parser_feed(parser, content, static_cast<size_t>(content_len));
    cmark_node* cmark_root = cmark_parser_finish(parser);
    Node* ase_root = convert_node(doc, cmark_root);
    cmark_node_free(cmark_root);
    cmark_parser_free(parser);
    return ase_root;
}

}  // namespace ase::markdown
