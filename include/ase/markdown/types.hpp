#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the ase-markdown node type ordinals
 * @description Every value a parse pass writes into Node::type and every mode the caller may
 *              request lives here and nowhere else. A second spelling of one of these numbers
 *              would be a second truth: nothing compares two tables of ordinals, so they drift
 *              in silence and a node reads as one kind on the writing side and another on the
 *              reading side. NO runtime values - those belong in the AST nodes themselves.
 *
 *              THE NUMBER BANDS ARE DELIBERATE AND THE GAPS ARE RESERVE:
 *                0-21   CommonMark plus GitHub Flavored Markdown - the standard surface,
 *                       produced in both parser modes
 *                30-39  ASE extensions available in BOTH modes (callouts, math, diagram
 *                       blocks, icons)
 *                50-56  the CMS DSL, reachable in DSGN mode only
 *              A new standard node goes into the first band, not onto the end of the list -
 *              that is what keeps "is this a DSL node" answerable from the number alone,
 *              without a table lookup.
 *
 *              ABBREVIATIONS: DSGN = design mode (the CMS DSL), TECH = technical mode
 *              (standard Markdown only), GFM = GitHub Flavored Markdown, ATX/setext = the two
 *              CommonMark heading syntaxes, SSOT = Single Source of Truth.
 *
 * @module      ase-markdown
 * @layer       1 (Core)
 * @created     2026-04-12
 * @modified    2026-08-20
 * @version     1.0.0
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] NO structs (structs belong in Components)
 * [ ] Abbreviations documented
 */

#include <cstdint>

namespace ase::markdown {

/**
 * AST NODE TYPES - band 0-21: CommonMark + GFM, produced in both parser modes
 */
constexpr uint8_t NODE_DOCUMENT         = 0;   // Tree root, exactly one per Document
constexpr uint8_t NODE_HEADING          = 1;   // ATX or setext heading, level carried on the node
constexpr uint8_t NODE_PARAGRAPH        = 2;   // Text block between blank lines
constexpr uint8_t NODE_TEXT             = 3;   // Literal run - the only node carrying raw text
constexpr uint8_t NODE_SOFTBREAK        = 4;   // Newline inside a paragraph, renders as a space
constexpr uint8_t NODE_LINEBREAK        = 5;   // Hard break: two trailing spaces or a backslash
constexpr uint8_t NODE_CODE_BLOCK       = 6;   // Fenced or indented, info string unclassified
constexpr uint8_t NODE_INLINE_CODE      = 7;   // Backtick span, content is never re-parsed
constexpr uint8_t NODE_BLOCKQUOTE       = 8;   // "> " container, nests arbitrarily
constexpr uint8_t NODE_LIST             = 9;   // Ordered or bullet container of list items
constexpr uint8_t NODE_LIST_ITEM        = 10;  // One entry - holds blocks, not text directly
constexpr uint8_t NODE_TABLE            = 11;  // GFM table container
constexpr uint8_t NODE_TABLE_ROW        = 12;  // One row; the header row is the first child
constexpr uint8_t NODE_TABLE_CELL       = 13;  // One cell, alignment from its column's ALIGN_*
constexpr uint8_t NODE_THEMATIC_BREAK   = 14;  // Horizontal rule, a leaf without children
constexpr uint8_t NODE_LINK             = 15;  // Inline or reference link
constexpr uint8_t NODE_IMAGE            = 16;  // Same shape as a link, rendered as media
constexpr uint8_t NODE_EMPHASIS         = 17;  // Single marker run
constexpr uint8_t NODE_STRONG           = 18;  // Double marker run
constexpr uint8_t NODE_STRIKETHROUGH    = 19;  // GFM tilde run
constexpr uint8_t NODE_HTML_BLOCK       = 20;  // Raw block, passed through untouched
constexpr uint8_t NODE_HTML_INLINE      = 21;  // Raw inline span, passed through untouched

/**
 * ASE SHARED EXTENSIONS - band 30-39: available in TECH and DSGN alike
 */
constexpr uint8_t NODE_FRONTMATTER      = 30;  // YAML header, first child of the document
constexpr uint8_t NODE_CALLOUT          = 31;  // Admonition box, kind from the CALLOUT_* values
constexpr uint8_t NODE_MATH_INLINE      = 32;  // KaTeX span, single dollar delimiters
constexpr uint8_t NODE_MATH_DISPLAY     = 33;  // KaTeX block, double dollar delimiters
constexpr uint8_t NODE_NERDFONT_ICON    = 34;  // Icon reference, resolved by the consumer
constexpr uint8_t NODE_MERMAID_BLOCK    = 35;  // Code block classified as mermaid
constexpr uint8_t NODE_DIFF_BLOCK       = 36;  // Code block classified as diff
constexpr uint8_t NODE_SVGBOB_BLOCK     = 37;  // Code block classified as svgbob
constexpr uint8_t NODE_ASEMATH_BLOCK    = 38;  // Code block classified as ase-math
constexpr uint8_t NODE_PLANTUML_BLOCK   = 39;  // Code block classified as plantuml

/**
 * ASE DSL EXTENSIONS - band 50-56: never produced in TECH mode
 */
constexpr uint8_t NODE_BLOCK_DIRECTIVE  = 50;  // :::name{attrs} ... ::: container
constexpr uint8_t NODE_LEAF_DIRECTIVE   = 51;  // ::name{attrs} leaf, carries no body
constexpr uint8_t NODE_TEXT_DIRECTIVE   = 52;  // :name[text]{attrs} inline form
constexpr uint8_t NODE_WIKI_LINK        = 53;  // [[target]] internal reference
constexpr uint8_t NODE_GLOSSARY_TERM    = 54;  // {{term}} glossary lookup
constexpr uint8_t NODE_CROSS_REF        = 55;  // {ref:id} pointer to another document
constexpr uint8_t NODE_VERSION_INSERT   = 56;  // {version} placeholder filled at render time

/**
 * CALLOUT KINDS - the flavour of a NODE_CALLOUT
 */
constexpr uint8_t CALLOUT_INFO          = 0;   // Neutral note, no consequence attached
constexpr uint8_t CALLOUT_WARNING       = 1;   // States what happens if it is ignored
constexpr uint8_t CALLOUT_TIP           = 2;   // Optional improvement, safe to skip
constexpr uint8_t CALLOUT_NOTE          = 3;   // Aside without a call to action

/**
 * TABLE CELL ALIGNMENT - per COLUMN, read from the GFM delimiter row
 */
constexpr uint8_t ALIGN_NONE            = 0;   // No colon in the delimiter - renderer decides
constexpr uint8_t ALIGN_LEFT            = 1;   // Leading colon
constexpr uint8_t ALIGN_CENTER          = 2;   // Colons on both sides
constexpr uint8_t ALIGN_RIGHT           = 3;   // Trailing colon

/**
 * LIMITS
 */
constexpr uint32_t MAX_FRONTMATTER_KEY_LEN = 32;  // Upper bound for a frontmatter key scan; the keys are literals in this module and the longest is "description" (11) - the bound exists so no scan is unbounded, not to cap the vocabulary

/**
 * PARSER MODES - what the caller requests; decides whether the 50-56 band can appear
 */
constexpr uint8_t MODE_TECH             = 0;   // CommonMark + GFM + the shared ASE extensions
constexpr uint8_t MODE_DSGN             = 1;   // All of the above plus the CMS DSL

}  // namespace ase::markdown
