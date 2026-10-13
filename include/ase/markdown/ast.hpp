#pragma once

/*
 * ==============================================================================
 * ASE CORE INFRASTRUCTURE HEADER
 * ==============================================================================
 *
 * @file        ast.hpp
 * @brief       The POD node type of the ase-markdown syntax tree
 * @description Declares Node, the single tree type every parse pass writes into,
 *              and Attr, the key/value pair a directive carries. Both are plain
 *              data: no methods, no virtuals, no ownership. The tree lives in the
 *              arena of its Document (ase-alloc), so a node holds raw child and
 *              sibling pointers into that arena and never frees anything itself -
 *              freeing one document is freeing one arena.
 *
 *              WHY THE POINTERS ARE RAW AND STAY RAW: the lifetime of every node
 *              is exactly the lifetime of its Document. A smart pointer would
 *              claim an ownership that does not exist here and would cost a
 *              refcount per node on a tree that is built once and read many
 *              times.
 *
 * @module      ase-markdown
 * @layer       1 (Core)
 * @category    process/computation/algorithm
 * @usecase     usecase:100008 - render the knowledge stock
 *
 * @created     2026-04-12
 * @modified    2026-10-13
 * @version     1.1.0
 *
 * ==============================================================================
 * CORE INFRASTRUCTURE COMPLIANCE
 * ==============================================================================
 * [ ] NOT an ECS Component or System
 * [ ] Layer dependencies correct (L0: no ASE deps, L1: L0 only)
 * [ ] No global mutable state (constexpr/const only)
 * [ ] No singletons or static mutable variables
 * [ ] Thread-safe by design (pure functions or explicit mutex)
 * [ ] All public functions documented with @brief, @param, @return
 * [ ] constexpr where possible (compile-time evaluation)
 * [ ] noexcept where possible (no-throw guarantee)
 * [ ] [[nodiscard]] on functions returning values
 * [ ] No magic numbers (use named constants)
 * [ ] No implicit conversions (use explicit constructors)
 * [ ] Header-only OR header+cpp pattern (not mixed)
 * [ ] Include guards via #pragma once
 * [ ] Namespace matches module: ase::{module}
 * [ ] No circular dependencies
 * [ ] No macros (except include guards) - use constexpr/templates
 * [ ] API stable (changes require version bump)
 * ==============================================================================
 */

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

// THE ARENA IS SIZED FROM THE INPUT, NEVER FROM A CONSTANT. A constant cannot
// bound a variable input: the document one byte past it finds the arena full,
// and what a full arena hands back is a nullptr in every alloc_node below. The
// size question is therefore a crash question, not a tuning question.
//
// The demand is proportional to the input, not equal to it. convert_node copies
// every literal into the arena, and the three splitting passes (icons, math,
// inline extensions) allocate a fresh node plus a fresh string per fragment
// while the arena never frees - so the text of a split run lives in the arena
// more than once, next to 112 bytes of Node per fragment.
//
// FACTOR CARRIES THAT MULTIPLE AS A STARTING POINT, NOT AS A PROMISE: parse()
// re-runs the whole parse against a doubled arena when a pass reports
// exhaustion, which is why no single number here has to be right.
//
// AND NO SINGLE NUMBER IS RIGHT FOR BOTH SHAPES. A table-heavy document of a
// quarter megabyte - every cell its own node with its own text, every cell with
// an icon that splits that text again - needs between twelve and twenty-four
// times its own length, while prose of the same size fits in a fraction of it.
// FACTOR sits at the prose end on purpose: the re-run costs one further cmark
// pass over a few hundred kilobytes, where a FACTOR raised to the table end
// would cost that memory on every document that never needed it.
//
// CEILING is where the retry stops and the parse reports instead of growing, so
// that one crafted document cannot ask the process for every byte it has. It is
// set against the stock this parser has to read, not against the address space:
// the largest markdown file of this tree is 3,617,006 bytes, and at the upper
// end of the measured demand that document needs some 87 MiB. A ceiling below
// that would turn the largest real document into an incomplete one, which is
// the failure a documentation viewer must not have; a ceiling far above it
// would trade a reported document for a host that swaps.
constexpr uint32_t DOCUMENT_ARENA_FLOOR   = 1024 * 1024;        // 1 MiB, the fixed cost of any document
constexpr uint32_t DOCUMENT_ARENA_FACTOR  = 8;                  // first guess, per input byte
constexpr uint32_t DOCUMENT_ARENA_CEILING = 128 * 1024 * 1024;  // 128 MiB, the retry stops here

// First arena size for a document of input_len bytes, saturating at the ceiling.
[[nodiscard]] inline uint32_t document_arena_size(uint32_t input_len) {
    if (input_len > (DOCUMENT_ARENA_CEILING - DOCUMENT_ARENA_FLOOR) / DOCUMENT_ARENA_FACTOR) {
        return DOCUMENT_ARENA_CEILING;
    }
    return input_len * DOCUMENT_ARENA_FACTOR + DOCUMENT_ARENA_FLOOR;
}

// Next arena size after an exhausted run, saturating at the ceiling.
[[nodiscard]] inline uint32_t document_arena_grown(uint32_t current) {
    if (current >= DOCUMENT_ARENA_CEILING / 2) return DOCUMENT_ARENA_CEILING;
    return current * 2;
}

// Document — owns buffer + arena for all nodes and strings
struct Document {
    Node* root              = nullptr;
    Frontmatter frontmatter = {};

    // Owned buffer + arena (ase::alloc::Arena from L0 ase-alloc)
    char* buffer            = nullptr;
    uint32_t buffer_size    = 0;
    ase::alloc::Arena* arena = nullptr;

    // 1 = an allocation found the arena full, so this tree is less than the
    // document. The two helpers below set it at the ONE place the failure
    // happens: no call site has to report it and none can forget to. parse()
    // reads it to decide whether to re-run against a larger arena, a renderer
    // reads it to say that what it draws is incomplete.
    uint8_t arena_exhausted = 0;
};

// Helper: allocate a Node from the document arena (zero-initialized)
inline Node* alloc_node(Document& doc, uint8_t type) {
    if (doc.arena == nullptr) {
        doc.arena_exhausted = 1;
        return nullptr;
    }
    auto* node = static_cast<Node*>(doc.arena->allocate_zeroed(sizeof(Node)));
    if (node == nullptr) {
        doc.arena_exhausted = 1;
        return nullptr;
    }
    node->type = type;
    return node;
}

// Helper: duplicate a string into the document arena
// An absent or empty source yields nullptr WITHOUT marking the document: only a
// full arena is a failure, and a node with no text is a legitimate node.
inline char* alloc_string(Document& doc, const char* str, uint32_t len) {
    if (!str || len == 0) return nullptr;
    if (doc.arena == nullptr) {
        doc.arena_exhausted = 1;
        return nullptr;
    }
    auto* copy = static_cast<char*>(doc.arena->allocate(len + 1));
    if (copy == nullptr) {
        doc.arena_exhausted = 1;
        return nullptr;
    }
    for (uint32_t i = 0; i < len; ++i) copy[i] = str[i];
    copy[len] = '\0';
    return copy;
}

// Helper: allocate an Attr array from the document arena (zero-initialized)
// Directive attributes are the one payload that is neither a node nor a string,
// and it goes through a helper for the same reason the other two do: an arena
// touched directly from a call site is an arena whose failure nobody records.
inline Attr* alloc_attrs(Document& doc, uint16_t count) {
    if (count == 0) return nullptr;
    if (doc.arena == nullptr) {
        doc.arena_exhausted = 1;
        return nullptr;
    }
    const uint32_t bytes = static_cast<uint32_t>(count) * static_cast<uint32_t>(sizeof(Attr));
    auto* attrs = static_cast<Attr*>(doc.arena->allocate_zeroed(bytes));
    if (attrs == nullptr) {
        doc.arena_exhausted = 1;
        return nullptr;
    }
    return attrs;
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
