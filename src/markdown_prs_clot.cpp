/**
 * Callout post-processing pass.
 *
 * Walks the AST looking for blockquotes whose first text starts with
 * [!INFO], [!WARNING], [!TIP], or [!NOTE]. Converts matching blockquotes
 * to NODE_CALLOUT with the appropriate callout_type.
 */

#include <ase/markdown/ast.hpp>
#include <cstring>

namespace ase::markdown {

namespace {

struct CalloutMatch {
    uint8_t type = 0;
    uint32_t marker_len = 0;
};

CalloutMatch detect_callout(const char* text, uint32_t len) {
    if (len < 7 || text[0] != '[' || text[1] != '!') return {};

    if (len >= 7  && std::memcmp(text, "[!INFO]", 7) == 0)    return { CALLOUT_INFO, 7 };
    if (len >= 10 && std::memcmp(text, "[!WARNING]", 10) == 0) return { CALLOUT_WARNING, 10 };
    if (len >= 6  && std::memcmp(text, "[!TIP]", 6) == 0)     return { CALLOUT_TIP, 6 };
    if (len >= 7  && std::memcmp(text, "[!NOTE]", 7) == 0)    return { CALLOUT_NOTE, 7 };

    return {};
}

// Find first text node in subtree (depth-first)
Node* find_first_text(Node* node) {
    if (!node) return nullptr;
    if (node->type == NODE_TEXT && node->text && node->text_len > 0) return node;
    Node* child = node->first_child;
    while (child) {
        Node* found = find_first_text(child);
        if (found) return found;
        child = child->next_sibling;
    }
    return nullptr;
}

void process_callouts(Node* node, Document& doc) {
    if (!node) return;

    if (node->type == NODE_BLOCKQUOTE) {
        Node* text_node = find_first_text(node);
        if (text_node) {
            auto match = detect_callout(text_node->text, text_node->text_len);
            if (match.marker_len > 0) {
                node->type = NODE_CALLOUT;
                node->callout_type = match.type;

                // Strip marker from text (skip marker + optional whitespace/newline)
                uint32_t skip = match.marker_len;
                while (skip < text_node->text_len &&
                       (text_node->text[skip] == ' ' || text_node->text[skip] == '\n')) skip++;

                if (skip < text_node->text_len) {
                    text_node->text = alloc_string(doc, text_node->text + skip, text_node->text_len - skip);
                    text_node->text_len -= skip;
                } else {
                    text_node->text = nullptr;
                    text_node->text_len = 0;
                }
            }
        }
    }

    Node* child = node->first_child;
    while (child) {
        process_callouts(child, doc);
        child = child->next_sibling;
    }
}

}  // anonymous namespace

void pass_callouts(Document& doc) {
    process_callouts(doc.root, doc);
}

}  // namespace ase::markdown
