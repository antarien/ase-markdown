/*
 * ==============================================================================
 * ASE CORE INFRASTRUCTURE IMPLEMENTATION
 * ==============================================================================
 *
 * @file        markdown_prs_math.cpp
 * @brief       Math post-processing pass — KaTeX spans and blocks out of text
 * @description Walks text nodes for $...$ and $$...$$, splits the run at each
 *              delimiter pair and inserts NODE_MATH_INLINE or NODE_MATH_DISPLAY
 *              between the remaining fragments. Display math is recognised when
 *              a paragraph holds nothing but the $$...$$ pair; inline math is
 *              recognised inside running text.
 *
 *              THE SPLIT IS WHY THIS RUNS ON TEXT NODES AND NOT ON THE INPUT:
 *              a dollar sign inside inline code or inside an HTML span is not a
 *              delimiter. By the time the AST exists those runs are already
 *              their own node types, so scanning text nodes alone cannot reach
 *              them - a regex over the raw input would have to re-implement
 *              that distinction and would get it wrong at the first edge case.
 *
 * @module      ase-markdown
 * @layer       1 (Core)
 * @category    process/computation/algorithm
 *
 * @created     2026-04-12
 * @modified    2026-08-20
 * @version     1.0.0
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

#include <ase/markdown/ast.hpp>
#include <cstring>

namespace ase::markdown {

namespace {

void process_math_in_text(Node* node, Document& doc) {
    if (!node) return;

    Node* child = node->first_child;
    while (child) {
        Node* next = child->next_sibling;
        process_math_in_text(child, doc);
        child = next;
    }

    if (node->type != NODE_TEXT || !node->text || node->text_len == 0) return;

    const char* text = node->text;
    uint32_t len = node->text_len;
    uint32_t pos = 0;
    uint32_t last_end = 0;
    Node* first_new = nullptr;
    Node* last_new = nullptr;

    auto append = [&](Node* n) {
        if (!first_new) first_new = n; else last_new->next_sibling = n;
        last_new = n;
    };

    while (pos < len) {
        if (text[pos] == '$') {
            bool is_display = (pos + 1 < len && text[pos + 1] == '$');
            uint32_t delim_len = is_display ? 2 : 1;
            uint32_t start = pos + delim_len;

            // Find closing delimiter
            uint32_t end = start;
            while (end < len) {
                if (text[end] == '$') {
                    if (is_display && end + 1 < len && text[end + 1] == '$') break;
                    if (!is_display && !(end + 1 < len && text[end + 1] == '$')) break;
                }
                end++;
            }

            if (end < len && end > start) {
                // Text before math
                if (pos > last_end) {
                    auto* txt = alloc_node(doc, NODE_TEXT);
                    txt->text = alloc_string(doc, text + last_end, pos - last_end);
                    txt->text_len = pos - last_end;
                    append(txt);
                }

                // Math node
                auto* math = alloc_node(doc, is_display ? NODE_MATH_DISPLAY : NODE_MATH_INLINE);
                math->text = alloc_string(doc, text + start, end - start);
                math->text_len = end - start;
                append(math);

                last_end = end + delim_len;
                pos = last_end;
                continue;
            }
        }
        pos++;
    }

    if (!first_new) return;

    // Remaining text
    if (last_end < len) {
        auto* txt = alloc_node(doc, NODE_TEXT);
        txt->text = alloc_string(doc, text + last_end, len - last_end);
        txt->text_len = len - last_end;
        append(txt);
    }

    // Replace original node, chain rest as siblings
    node->type = first_new->type;
    node->text = first_new->text;
    node->text_len = first_new->text_len;

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

void pass_math(Document& doc) {
    process_math_in_text(doc.root, doc);
}

}  // namespace ase::markdown
