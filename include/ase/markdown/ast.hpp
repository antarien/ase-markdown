#pragma once

#include <ase/markdown/types.hpp>
#include <cstdint>

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

// Document — owns all memory (arena-based allocation)
struct Document {
    Node* root              = nullptr;
    Frontmatter frontmatter = {};

    // Arena for all strings and nodes (freed in free_document)
    void* arena             = nullptr;
};

}  // namespace ase::markdown
