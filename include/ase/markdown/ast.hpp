#pragma once

#include <ase/markdown/types.hpp>
#include <ase/alloc/arena.hpp>
#include <cstdint>
#include <cstdlib>

namespace ase::markdown {

// Forward declaration
struct Node;

// Key-value pair for directive attributes
struct Attr {
    const char* key   = nullptr;
    const char* value = nullptr;
};

// AST Node — POD structure, no methods, no virtuals
struct Node {
    uint8_t type          = NODE_DOCUMENT;

    // Tree pointers (owned by Document arena)
    Node* first_child     = nullptr;
    Node* next_sibling    = nullptr;
    Node* parent          = nullptr;

    // Text content (points into Document string arena)
    const char* text      = nullptr;
    uint32_t text_len     = 0;

    // Type-specific fields
    uint8_t heading_level = 0;       // NODE_HEADING: 1-6
    uint8_t callout_type  = 0;       // NODE_CALLOUT: CALLOUT_INFO/WARNING/TIP/NOTE
    uint8_t alignment     = 0;       // NODE_TABLE_CELL: ALIGN_*
    uint8_t list_ordered  = 0;       // NODE_LIST: 0=bullet, 1=ordered
    uint32_t list_start   = 1;       // NODE_LIST: start number

    // Code block / directive fields
    const char* language  = nullptr;  // NODE_CODE_BLOCK: "cpp", "diff", etc.
    const char* info      = nullptr;  // Additional info string

    // Directive fields (DSGN mode)
    const char* directive_name = nullptr;  // NODE_BLOCK_DIRECTIVE/LEAF/TEXT: "columns", "badge", etc.
    Attr* attrs           = nullptr;  // Directive attributes array
    uint16_t attr_count   = 0;

    // Link/image fields
    const char* url       = nullptr;  // NODE_LINK/IMAGE: href/src
    const char* title     = nullptr;  // NODE_LINK/IMAGE: title attribute
};

// Frontmatter key-value store
struct Frontmatter {
    const char* title       = nullptr;
    const char* description = nullptr;
    const char* version     = nullptr;
    const char* icon        = nullptr;
    const char* image       = nullptr;
    const char* category    = nullptr;
    const char* date        = nullptr;
    const char** keywords   = nullptr;
    uint16_t keyword_count  = 0;
    int32_t order           = 0;
    uint8_t curated         = 0;
};

// Default arena size: 1MB (sufficient for large documents)
constexpr uint32_t DOCUMENT_ARENA_SIZE = 1024 * 1024;

// Document — owns buffer + arena for all nodes and strings
struct Document {
    Node* root              = nullptr;
    Frontmatter frontmatter = {};

    // Owned buffer + arena (ase::alloc::Arena from L0 ase-alloc)
    char* buffer            = nullptr;
    uint32_t buffer_size    = 0;
    ase::alloc::Arena* arena = nullptr;
};

// Helper: allocate a Node from the document arena (zero-initialized)
inline Node* alloc_node(Document& doc, uint8_t type) {
    auto* node = static_cast<Node*>(doc.arena->allocate_zeroed(sizeof(Node)));
    if (node) node->type = type;
    return node;
}

// Helper: duplicate a string into the document arena
inline char* alloc_string(Document& doc, const char* str, uint32_t len) {
    if (!str || len == 0) return nullptr;
    auto* copy = static_cast<char*>(doc.arena->allocate(len + 1));
    if (!copy) return nullptr;
    for (uint32_t i = 0; i < len; ++i) copy[i] = str[i];
    copy[len] = '\0';
    return copy;
}

// Helper: append child node to parent
inline void append_child(Node* parent, Node* child) {
    child->parent = parent;
    if (!parent->first_child) {
        parent->first_child = child;
    } else {
        Node* last = parent->first_child;
        while (last->next_sibling) last = last->next_sibling;
        last->next_sibling = child;
    }
}

}  // namespace ase::markdown
