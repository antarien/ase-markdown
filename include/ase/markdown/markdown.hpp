#pragma once

/*
 * ==============================================================================
 * ASE CORE INFRASTRUCTURE HEADER
 * ==============================================================================
 *
 * @file        markdown.hpp
 * @brief       ase-markdown umbrella include — CommonMark/GFM + ASE DSL parser
 * @description Public surface of ase-markdown. Parses Markdown input into a
 *              typed POD AST with support for standard CommonMark + GitHub
 *              Flavored Markdown (via cmark-gfm), callout boxes, KaTeX math,
 *              NerdFont icons, code block classification (mermaid, diff,
 *              svgbob, ase-math) and YAML frontmatter. In DSGN mode it also
 *              carries the CMS DSL: 21 block directives, 10 leaf directives
 *              and 7 inline extensions. The directive NAME registry that the
 *              DSL pass writes into Node::directive_name is re-exported here
 *              (directives.hpp), so a consumer dispatching on directive names
 *              reaches the constants through this single umbrella instead of
 *              repeating the string literals.
 *
 * @module      ase-markdown
 * @layer       1 (Core)
 * @category    process/computation/algorithm
 *
 * @created     2026-04-12
 * @modified    2026-08-18
 * @version     00.00.18.00018 [init]
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
#include <ase/markdown/ast.hpp>
#include <ase/markdown/directives.hpp>
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
