/**
 * Directive pre-processing pass.
 *
 * Runs BEFORE cmark-gfm parsing. Converts DSL syntax to HTML comments
 * that survive cmark parsing, then post-processes to create directive nodes.
 *
 * Block directives:  :::name{attrs} ... :::
 * Leaf directives:   ::name{attrs}
 * Inline extensions: [[wiki]], {{glossary}}, {ref:}, {icon:}, {version}, :tip[]{}
 *
 * Only active in DSGN mode.
 */

#include <ase/markdown/ast.hpp>
#include <cstring>

namespace ase::markdown {

namespace {

// Parse attribute string: key1="val1" key2="val2" → Attr array
uint16_t parse_attrs(const char* attr_str, uint32_t attr_len, Document& doc, Attr*& out) {
    if (!attr_str || attr_len == 0) { out = nullptr; return 0; }

    // Count approximate number of attrs (count '=' signs)
    uint16_t count = 0;
    for (uint32_t i = 0; i < attr_len; i++) {
        if (attr_str[i] == '=') count++;
    }
    // Also count flag attrs (words without =)
    if (count == 0) count = 1;

    out = static_cast<Attr*>(doc.arena->allocate(count * sizeof(Attr)));
    uint16_t actual = 0;
    uint32_t pos = 0;

    while (pos < attr_len && actual < count) {
        // Skip whitespace
        while (pos < attr_len && (attr_str[pos] == ' ' || attr_str[pos] == '\t')) pos++;
        if (pos >= attr_len) break;

        // Read key
        uint32_t key_start = pos;
        while (pos < attr_len && attr_str[pos] != '=' && attr_str[pos] != ' ' && attr_str[pos] != '\t') pos++;
        uint32_t key_len = pos - key_start;
        if (key_len == 0) break;

        char* key = alloc_string(doc, attr_str + key_start, key_len);

        // Check for = (key=value) vs flag (key only)
        if (pos < attr_len && attr_str[pos] == '=') {
            pos++;
            // Read value (quoted or unquoted)
            char* val = nullptr;
            if (pos < attr_len && (attr_str[pos] == '"' || attr_str[pos] == '\'')) {
                char quote = attr_str[pos];
                pos++;
                uint32_t val_start = pos;
                while (pos < attr_len && attr_str[pos] != quote) pos++;
                val = alloc_string(doc, attr_str + val_start, pos - val_start);
                if (pos < attr_len) pos++; // skip closing quote
            } else {
                uint32_t val_start = pos;
                while (pos < attr_len && attr_str[pos] != ' ' && attr_str[pos] != '\t') pos++;
                val = alloc_string(doc, attr_str + val_start, pos - val_start);
            }
            out[actual] = { key, val };
        } else {
            // Flag attribute (e.g., "lightbox", "highlight")
            out[actual] = { key, key };
        }
        actual++;
    }

    return actual;
}

// Scan for block directive: :::name{attrs}\n...\n:::\n
// Returns true if found, sets name/attrs/content/end positions
bool scan_block_directive(const char* text, uint32_t pos, uint32_t len,
                          uint32_t& name_start, uint32_t& name_len,
                          uint32_t& attr_start, uint32_t& attr_len,
                          uint32_t& content_start, uint32_t& content_end,
                          uint32_t& block_end) {
    // Must start with :::
    if (pos + 3 >= len || text[pos] != ':' || text[pos+1] != ':' || text[pos+2] != ':') return false;

    // Read name
    name_start = pos + 3;
    uint32_t p = name_start;
    while (p < len && text[p] != '{' && text[p] != '\n' && text[p] != ' ') p++;
    name_len = p - name_start;
    if (name_len == 0) return false;

    // Read optional {attrs}
    attr_start = 0;
    attr_len = 0;
    if (p < len && text[p] == '{') {
        attr_start = p + 1;
        while (p < len && text[p] != '}') p++;
        attr_len = p - attr_start;
        if (p < len) p++; // skip }
    }

    // Skip to end of opening line
    while (p < len && text[p] != '\n') p++;
    if (p < len) p++;
    content_start = p;

    // Find closing :::
    while (p < len) {
        if (p + 2 < len && text[p] == ':' && text[p+1] == ':' && text[p+2] == ':') {
            // Check it's at line start (p == 0 or text[p-1] == '\n')
            if (p == 0 || text[p-1] == '\n') {
                content_end = p;
                block_end = p + 3;
                if (block_end < len && text[block_end] == '\n') block_end++;
                return true;
            }
        }
        while (p < len && text[p] != '\n') p++;
        if (p < len) p++;
    }

    return false;
}

}  // anonymous namespace

void pass_directives(Document& doc) {
    // Walk HTML block nodes — directives that survived cmark as HTML blocks
    // In the current implementation, directives are detected as part of
    // text content within paragraphs and blockquotes.
    //
    // For a full implementation, a pre-processing step would be needed
    // before cmark parsing to extract directives. For now, this pass
    // handles inline extensions within text nodes.

    // TODO: Implement full directive parsing in Phase 2.3
    // This requires pre-processing the raw input before cmark-gfm,
    // which will be done when the parser is integrated with the viewer.
    (void)doc;
}

// Parse inline extensions in text nodes: [[wiki]], {{glossary}}, {ref:}, {version}
void pass_inline_extensions(Document& doc) {
    (void)doc;
    // TODO: Implement in Phase 2.3 continuation
}

}  // namespace ase::markdown
