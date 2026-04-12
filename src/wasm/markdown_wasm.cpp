/**
 * WASM entry points for ase-markdown.
 *
 * Compiles with Emscripten to produce ase-markdown.wasm + ase-markdown.js.
 * Provides C-linkage functions callable from JavaScript:
 *   parse_markdown(input, len, mode) → JSON AST string
 *   free_result(ptr) → frees the JSON string
 *
 * Usage from JS:
 *   const AseMarkdown = await AseMarkdown();
 *   const parse = AseMarkdown.cwrap('parse_markdown', 'number', ['string', 'number', 'number']);
 *   const ptr = parse(mdText, mdText.length, 0); // 0=TECH, 1=DSGN
 *   const json = AseMarkdown.UTF8ToString(ptr);
 *   AseMarkdown._free_result(ptr);
 */

#include <ase/markdown/markdown.hpp>
#include <ase/markdown/types.hpp>
#include <cstring>
#include <cstdlib>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define WASM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define WASM_EXPORT
#endif

namespace {

void node_to_json(const ase::markdown::Node* node, std::string& out, int depth = 0) {
    if (!node) return;
    out += "{";
    out += "\"type\":" + std::to_string(node->type);
    if (node->text && node->text_len > 0) {
        out += ",\"text\":\"";
        for (uint32_t i = 0; i < node->text_len; i++) {
            char c = node->text[i];
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else out += c;
        }
        out += "\"";
    }
    if (node->heading_level > 0) out += ",\"level\":" + std::to_string(node->heading_level);
    if (node->callout_type > 0) out += ",\"callout\":" + std::to_string(node->callout_type);
    if (node->language) out += ",\"lang\":\"" + std::string(node->language) + "\"";
    if (node->directive_name) out += ",\"directive\":\"" + std::string(node->directive_name) + "\"";
    if (node->url) out += ",\"url\":\"" + std::string(node->url) + "\"";

    if (node->first_child) {
        out += ",\"children\":[";
        const ase::markdown::Node* child = node->first_child;
        bool first = true;
        while (child) {
            if (!first) out += ",";
            node_to_json(child, out, depth + 1);
            first = false;
            child = child->next_sibling;
        }
        out += "]";
    }
    out += "}";
}

}  // anonymous namespace

extern "C" {

WASM_EXPORT
char* parse_markdown(const char* input, uint32_t len, uint8_t mode) {
    ase::markdown::ParseOptions opts{};
    opts.mode = mode;
    opts.parse_frontmatter = 1;
    auto doc = ase::markdown::parse(input, len, opts);

    std::string json;
    json += "{\"frontmatter\":{";
    if (doc.frontmatter.title) json += "\"title\":\"" + std::string(doc.frontmatter.title) + "\"";
    json += "},\"root\":";
    if (doc.root) {
        node_to_json(doc.root, json);
    } else {
        json += "null";
    }
    json += "}";

    ase::markdown::free_document(doc);

    char* result = static_cast<char*>(std::malloc(json.size() + 1));
    std::memcpy(result, json.c_str(), json.size() + 1);
    return result;
}

WASM_EXPORT
void free_result(char* ptr) {
    std::free(ptr);
}

}  // extern "C"
