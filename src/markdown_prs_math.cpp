/**
 * Math post-processing pass.
 *
 * Walks text nodes looking for $...$ (inline math) and $$...$$ (display math).
 * Splits text nodes and inserts NODE_MATH_INLINE / NODE_MATH_DISPLAY nodes.
 *
 * Display math ($$) is detected on separate lines (paragraph containing only $$...$$).
 * Inline math ($) is detected within running text.
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
