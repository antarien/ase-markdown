/**
 * NerdFont icon post-processing pass.
 *
 * Walks all text nodes looking for (nf-*-*) patterns.
 * Splits text nodes around icon patterns, inserting NODE_NERDFONT_ICON nodes.
 */

#include <ase/markdown/ast.hpp>
#include <cstring>

namespace ase::markdown {

namespace {

// Check if position in text starts an icon pattern: (nf-
bool is_icon_start(const char* text, uint32_t pos, uint32_t len) {
    return pos + 4 < len && text[pos] == '(' && text[pos + 1] == 'n' &&
           text[pos + 2] == 'f' && text[pos + 3] == '-';
}

// Find closing ) for icon pattern starting at pos
// Returns position of ) or 0 if not found
uint32_t find_icon_end(const char* text, uint32_t pos, uint32_t len) {
    uint32_t i = pos + 4;
    while (i < len) {
        if (text[i] == ')') return i;
        // Icon names contain: a-z, 0-9, -, _
        char c = text[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_')) return 0;
        i++;
    }
    return 0;
}

void process_icons(Node* node, Document& doc) {
    if (!node) return;

    // Process children first (avoid modifying list while iterating)
    Node* child = node->first_child;
    while (child) {
        Node* next = child->next_sibling;
        process_icons(child, doc);
        child = next;
    }

    // Only split text nodes
    if (node->type != NODE_TEXT || !node->text || node->text_len == 0) return;

    const char* text = node->text;
    uint32_t len = node->text_len;
    uint32_t pos = 0;
    uint32_t last_end = 0;
    Node* first_new = nullptr;
    Node* last_new = nullptr;

    while (pos < len) {
        if (is_icon_start(text, pos, len)) {
            uint32_t close = find_icon_end(text, pos, len);
            if (close > 0) {
                // Text before icon
                if (pos > last_end) {
                    auto* txt = alloc_node(doc, NODE_TEXT);
                    txt->text = alloc_string(doc, text + last_end, pos - last_end);
                    txt->text_len = pos - last_end;
                    if (!first_new) first_new = txt; else last_new->next_sibling = txt;
                    last_new = txt;
                }

                // Icon node (nf-fa-name) → text = "nf-fa-name"
                auto* icon = alloc_node(doc, NODE_NERDFONT_ICON);
                icon->text = alloc_string(doc, text + pos + 1, close - pos - 1);
                icon->text_len = close - pos - 1;
                if (!first_new) first_new = icon; else last_new->next_sibling = icon;
                last_new = icon;

                last_end = close + 1;
                pos = close + 1;
                continue;
            }
        }
        pos++;
    }

    if (!first_new) return;

    // Remaining text after last icon
    if (last_end < len) {
        auto* txt = alloc_node(doc, NODE_TEXT);
        txt->text = alloc_string(doc, text + last_end, len - last_end);
        txt->text_len = len - last_end;
        last_new->next_sibling = txt;
        last_new = txt;
    }

    // Replace original node's content with first fragment, chain rest as siblings
    node->type = first_new->type;
    node->text = first_new->text;
    node->text_len = first_new->text_len;

    // Insert remaining fragments after this node
    Node* tail = first_new->next_sibling;
    if (tail) {
        Node* orig_next = node->next_sibling;
        node->next_sibling = tail;
        // Find end of new chain
        Node* chain_end = tail;
        while (chain_end->next_sibling) chain_end = chain_end->next_sibling;
        chain_end->next_sibling = orig_next;
        // Set parent pointers
        Node* n = tail;
        while (n && n != orig_next) { n->parent = node->parent; n = n->next_sibling; }
    }
}

}  // anonymous namespace

void pass_icons(Document& doc) {
    process_icons(doc.root, doc);
}

}  // namespace ase::markdown
