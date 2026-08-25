/*
 * ==============================================================================
 * ASE CORE INFRASTRUCTURE IMPLEMENTATION
 * ==============================================================================
 *
 * @file        markdown_wasm.cpp
 * @brief       WASM entry point for ase-markdown — parses markdown into a JSON AST
 * @description Compiles with Emscripten to produce ase-markdown.wasm plus its
 *              loader ase-markdown.js. One function crosses to JavaScript: it
 *              takes the markdown text and the parse mode and returns the AST as
 *              a JSON string.
 *
 *              Usage from JS:
 *                const AseMarkdown = await AseMarkdown();
 *                const json = AseMarkdown.parse_markdown(mdText, 0); // 0=TECH, 1=DSGN
 *
 *              IT WENT THROUGH A C ABI UNTIL 2026-08-20, AND THAT IS WHY THE JS
 *              LINE ABOVE IS SHORTER THAN IT WAS. The old shape exported the two
 *              functions with C linkage, returned a raw char* from malloc, and
 *              left JavaScript responsible for calling free_result on it - a
 *              manual memory contract across a language boundary, where a missed
 *              call leaks and a double call corrupts the heap. Three compliance
 *              findings named exactly that construction (C linkage once,
 *              malloc/free twice), and all three were right: embind is the C++
 *              interface the rule asks for, and it marshals the returned
 *              std::string into a JS string itself. Nothing is allocated by hand
 *              here any more, and there is nothing left for a caller to free.
 *
 *              THE SWITCH COST NOTHING TODAY, MEASURED: no .ts, .tsx, .js, .json
 *              or .sh file in the tree calls parse_markdown or free_result, or
 *              loads ase-markdown.wasm. The C entry points had no caller, so
 *              changing the call shape broke no consumer. A future caller gets
 *              the shorter form above.
 *
 *              THE TWO WASM_EXPORT MACROS ARE GONE WITH IT. They chose between
 *              EMSCRIPTEN_KEEPALIVE and nothing, for a translation unit that
 *              CMake compiles only inside `if(EMSCRIPTEN)` (CMakeLists.txt:185) -
 *              the fallback branch could never be taken. embind registers the
 *              function itself, so no keep-alive attribute is needed at all.
 *
 * @module      ase-markdown
 * @layer       1 (Core)
 * @category    process/computation/algorithm
 *
 * @created     2026-04-12
 * @modified    2026-08-20
 * @version     2.0.0
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

#include <ase/markdown/markdown.hpp>

#include <ase/markdown/types.hpp>

#include <emscripten/bind.h>

#include <string>

namespace ase::markdown {

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

}  // namespace

/**
 * @brief Parse markdown and return the AST as a JSON string.
 * @param input The markdown source. embind copies it in from the JS string.
 * @param mode Parse mode: 0 = TECH, 1 = DSGN.
 * @return The JSON AST. embind copies it out into a JS string; the caller frees
 *         nothing, and the document built here is released before returning.
 */
std::string parse_to_json(const std::string& input, uint8_t mode) {
    ParseOptions opts{};
    opts.mode = mode;
    opts.parse_frontmatter = 1;
    auto doc = parse(input.c_str(), static_cast<uint32_t>(input.size()), opts);

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

    free_document(doc);
    return json;
}

}  // namespace ase::markdown

// The registration block embind requires at global scope. The JS-side name stays
// parse_markdown, which is what the old C export was called - only the call shape
// changed, not the name a future caller looks for.
EMSCRIPTEN_BINDINGS(ase_markdown) {
    emscripten::function("parse_markdown", &ase::markdown::parse_to_json);
}
