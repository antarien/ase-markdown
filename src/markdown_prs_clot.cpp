/*
 * ==============================================================================
 * ASE CORE INFRASTRUCTURE IMPLEMENTATION
 * ==============================================================================
 *
 * @file        markdown_prs_clot.cpp
 * @brief       Callout post-processing pass — blockquote to NODE_CALLOUT
 * @description Walks the AST for blockquotes whose first text run begins with
 *              [!INFO], [!WARNING], [!TIP] or [!NOTE] and rewrites those nodes
 *              to NODE_CALLOUT carrying the matching CALLOUT_* value.
 *
 *              WHY IT IS A PASS AND NOT PART OF THE BLOCK PARSER: a callout is
 *              a plain CommonMark blockquote until its first text is read, so
 *              recognising it inside the block parser would mean looking ahead
 *              into inline content that has not been parsed yet. Running it
 *              afterwards keeps the block parser standard-conformant and makes
 *              the ASE extension removable without touching it.
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
