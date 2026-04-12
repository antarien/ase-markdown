#include <ase/markdown/markdown.hpp>
#include <ase/markdown/frontmatter.hpp>
#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>
#include <cstdlib>
#include <cstring>

namespace ase::markdown {

namespace {

// Map cmark node type to ase::markdown node type
uint8_t map_node_type(cmark_node_type ctype) {
    switch (ctype) {
        case CMARK_NODE_DOCUMENT:       return NODE_DOCUMENT;
        case CMARK_NODE_HEADING:        return NODE_HEADING;
        case CMARK_NODE_PARAGRAPH:      return NODE_PARAGRAPH;
        case CMARK_NODE_TEXT:           return NODE_TEXT;
        case CMARK_NODE_SOFTBREAK:      return NODE_SOFTBREAK;
        case CMARK_NODE_LINEBREAK:      return NODE_LINEBREAK;
        case CMARK_NODE_CODE_BLOCK:     return NODE_CODE_BLOCK;
        case CMARK_NODE_CODE:           return NODE_INLINE_CODE;
        case CMARK_NODE_BLOCK_QUOTE:    return NODE_BLOCKQUOTE;
        case CMARK_NODE_LIST:           return NODE_LIST;
        case CMARK_NODE_ITEM:           return NODE_LIST_ITEM;
        case CMARK_NODE_THEMATIC_BREAK: return NODE_THEMATIC_BREAK;
        case CMARK_NODE_LINK:           return NODE_LINK;
        case CMARK_NODE_IMAGE:          return NODE_IMAGE;
        case CMARK_NODE_EMPH:           return NODE_EMPHASIS;
        case CMARK_NODE_STRONG:         return NODE_STRONG;
        case CMARK_NODE_HTML_BLOCK:     return NODE_HTML_BLOCK;
        case CMARK_NODE_HTML_INLINE:    return NODE_HTML_INLINE;
        default:                        return NODE_TEXT;
    }
}

// Recursively convert cmark node tree to ase::markdown node tree
Node* convert_node(Document& doc, cmark_node* cnode) {
    if (!cnode) return nullptr;

    uint8_t type = map_node_type(cmark_node_get_type(cnode));

    // Classify code blocks by language
    if (type == NODE_CODE_BLOCK) {
        const char* fence_info = cmark_node_get_fence_info(cnode);
        if (fence_info) {
            if (std::strcmp(fence_info, "mermaid") == 0)  type = NODE_MERMAID_BLOCK;
            else if (std::strcmp(fence_info, "diff") == 0)    type = NODE_DIFF_BLOCK;
            else if (std::strcmp(fence_info, "svgbob") == 0)  type = NODE_SVGBOB_BLOCK;
            else if (std::strcmp(fence_info, "ase-math") == 0) type = NODE_ASEMATH_BLOCK;
        }
    }

    // Handle GFM table extension
    if (cmark_node_get_type(cnode) == CMARK_NODE_TABLE) type = NODE_TABLE;
    else if (cmark_node_get_type(cnode) == CMARK_NODE_TABLE_ROW) type = NODE_TABLE_ROW;
    else if (cmark_node_get_type(cnode) == CMARK_NODE_TABLE_CELL) type = NODE_TABLE_CELL;

    // Handle GFM strikethrough
    if (cmark_node_get_type(cnode) == CMARK_NODE_STRIKETHROUGH) type = NODE_STRIKETHROUGH;

    Node* node = alloc_node(doc, type);
    if (!node) return nullptr;

    // Copy text content
    const char* literal = cmark_node_get_literal(cnode);
    if (literal) {
        uint32_t len = static_cast<uint32_t>(std::strlen(literal));
        node->text = alloc_string(doc, literal, len);
        node->text_len = len;
    }

    // Heading level
    if (type == NODE_HEADING) {
        node->heading_level = static_cast<uint8_t>(cmark_node_get_heading_level(cnode));
    }

    // List info
    if (type == NODE_LIST) {
        node->list_ordered = (cmark_node_get_list_type(cnode) == CMARK_ORDERED_LIST) ? 1 : 0;
        node->list_start = static_cast<uint32_t>(cmark_node_get_list_start(cnode));
    }

    // Code block language
    if (type == NODE_CODE_BLOCK || type == NODE_MERMAID_BLOCK ||
        type == NODE_DIFF_BLOCK || type == NODE_SVGBOB_BLOCK || type == NODE_ASEMATH_BLOCK) {
        const char* info = cmark_node_get_fence_info(cnode);
        if (info && info[0] != '\0') {
            node->language = alloc_string(doc, info, static_cast<uint32_t>(std::strlen(info)));
        }
    }

    // Link/image URL and title
    if (type == NODE_LINK || type == NODE_IMAGE) {
        const char* url = cmark_node_get_url(cnode);
        if (url) node->url = alloc_string(doc, url, static_cast<uint32_t>(std::strlen(url)));
        const char* title = cmark_node_get_title(cnode);
        if (title && title[0] != '\0') {
            node->title = alloc_string(doc, title, static_cast<uint32_t>(std::strlen(title)));
        }
    }

    // Recursively convert children
    cmark_node* child = cmark_node_first_child(cnode);
    while (child) {
        Node* child_node = convert_node(doc, child);
        if (child_node) {
            append_child(node, child_node);
        }
        child = cmark_node_next(child);
    }

    return node;
}

}  // anonymous namespace

Document parse(const char* input, uint32_t len, ParseOptions opts) {
    // Allocate document with arena
    Document doc{};
    doc.buffer_size = DOCUMENT_ARENA_SIZE;
    doc.buffer = static_cast<char*>(std::malloc(doc.buffer_size));
    doc.arena = static_cast<ase::alloc::Arena*>(std::malloc(sizeof(ase::alloc::Arena)));
    *doc.arena = ase::alloc::Arena(doc.buffer, doc.buffer_size);

    // Strip frontmatter before parsing markdown
    const char* body = input;
    uint32_t body_len = len;
    if (opts.parse_frontmatter) {
        uint32_t offset = parse_frontmatter(input, len, doc.frontmatter, doc);
        if (offset > 0) {
            body = input + offset;
            body_len = len - offset;
        }
    }

    // Register GFM extensions
    cmark_gfm_core_extensions_ensure_registered();

    // Create parser with GFM extensions
    cmark_parser* parser = cmark_parser_new(CMARK_OPT_DEFAULT);
    cmark_syntax_extension* table_ext = cmark_find_syntax_extension("table");
    cmark_syntax_extension* autolink_ext = cmark_find_syntax_extension("autolink");
    cmark_syntax_extension* strike_ext = cmark_find_syntax_extension("strikethrough");
    cmark_syntax_extension* tasklist_ext = cmark_find_syntax_extension("tasklist");

    if (table_ext) cmark_parser_attach_syntax_extension(parser, table_ext);
    if (autolink_ext) cmark_parser_attach_syntax_extension(parser, autolink_ext);
    if (strike_ext) cmark_parser_attach_syntax_extension(parser, strike_ext);
    if (tasklist_ext) cmark_parser_attach_syntax_extension(parser, tasklist_ext);

    // Parse body (without frontmatter)
    cmark_parser_feed(parser, body, static_cast<size_t>(body_len));
    cmark_node* cmark_root = cmark_parser_finish(parser);

    // Convert cmark tree to ase::markdown tree
    doc.root = convert_node(doc, cmark_root);

    // Cleanup cmark
    cmark_node_free(cmark_root);
    cmark_parser_free(parser);

    // Post-processing passes (ASE extensions)
    pass_callouts(doc);
    pass_math(doc);
    pass_icons(doc);

    return doc;
}

void free_document(Document& doc) {
    if (doc.arena) {
        std::free(doc.arena);
        doc.arena = nullptr;
    }
    if (doc.buffer) {
        std::free(doc.buffer);
        doc.buffer = nullptr;
    }
    doc.root = nullptr;
    doc.buffer_size = 0;
}

}  // namespace ase::markdown
