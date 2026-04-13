#pragma once

/**
 * ase-markdown — CommonMark/GFM + ASE DSL Markdown Parser
 *
 * Parses Markdown input into a typed AST with support for:
 * - Standard CommonMark + GitHub Flavored Markdown (via cmark-gfm)
 * - Callout boxes, KaTeX math, NerdFont icons
 * - Code block classification (mermaid, diff, svgbob, ase-math)
 * - YAML frontmatter
 * - CMS DSL directives (DSGN mode: 21 block + 10 leaf + 7 inline)
 *
 * @module   ase-markdown
 * @layer    1 (Core)
 */

#include <ase/markdown/types.hpp>
#include <ase/markdown/ast.hpp>
#include <string>

namespace ase::markdown {

struct ParseOptions {
    uint8_t mode               = MODE_TECH;
    uint8_t parse_frontmatter  = 1;
};

// Parse markdown input into a Document AST.
// Returns a Document with arena-allocated nodes.
// Caller must call free_document() when done.
Document parse(const char* input, uint32_t len, ParseOptions opts = {});

// Free all memory owned by a Document (nodes, strings, arena).
void free_document(Document& doc);

// Pre-processing (called by parse() before cmark-gfm, DSGN mode only)
std::string preprocess_directives(const char* input, uint32_t len);

// Post-processing passes (called by parse(), also available individually)
void pass_callouts(Document& doc);
void pass_icons(Document& doc);
void pass_math(Document& doc);
void pass_directives(Document& doc);
void pass_inline_extensions(Document& doc);

}  // namespace ase::markdown
