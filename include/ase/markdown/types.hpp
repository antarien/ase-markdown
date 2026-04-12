#pragma once

#include <cstdint>

namespace ase::markdown {

// AST node type identifiers
constexpr uint8_t NODE_DOCUMENT         = 0;
constexpr uint8_t NODE_HEADING          = 1;
constexpr uint8_t NODE_PARAGRAPH        = 2;
constexpr uint8_t NODE_TEXT             = 3;
constexpr uint8_t NODE_SOFTBREAK        = 4;
constexpr uint8_t NODE_LINEBREAK        = 5;
constexpr uint8_t NODE_CODE_BLOCK       = 6;
constexpr uint8_t NODE_INLINE_CODE      = 7;
constexpr uint8_t NODE_BLOCKQUOTE       = 8;
constexpr uint8_t NODE_LIST             = 9;
constexpr uint8_t NODE_LIST_ITEM        = 10;
constexpr uint8_t NODE_TABLE            = 11;
constexpr uint8_t NODE_TABLE_ROW        = 12;
constexpr uint8_t NODE_TABLE_CELL       = 13;
constexpr uint8_t NODE_THEMATIC_BREAK   = 14;
constexpr uint8_t NODE_LINK             = 15;
constexpr uint8_t NODE_IMAGE            = 16;
constexpr uint8_t NODE_EMPHASIS         = 17;
constexpr uint8_t NODE_STRONG           = 18;
constexpr uint8_t NODE_STRIKETHROUGH    = 19;
constexpr uint8_t NODE_HTML_BLOCK       = 20;
constexpr uint8_t NODE_HTML_INLINE      = 21;

// ASE shared extensions (TECH + DSGN)
constexpr uint8_t NODE_FRONTMATTER      = 30;
constexpr uint8_t NODE_CALLOUT          = 31;
constexpr uint8_t NODE_MATH_INLINE      = 32;
constexpr uint8_t NODE_MATH_DISPLAY     = 33;
constexpr uint8_t NODE_NERDFONT_ICON    = 34;
constexpr uint8_t NODE_MERMAID_BLOCK    = 35;
constexpr uint8_t NODE_DIFF_BLOCK       = 36;
constexpr uint8_t NODE_SVGBOB_BLOCK     = 37;
constexpr uint8_t NODE_ASEMATH_BLOCK    = 38;

// ASE DSL extensions (DSGN only)
constexpr uint8_t NODE_BLOCK_DIRECTIVE  = 50;
constexpr uint8_t NODE_LEAF_DIRECTIVE   = 51;
constexpr uint8_t NODE_TEXT_DIRECTIVE    = 52;
constexpr uint8_t NODE_WIKI_LINK        = 53;
constexpr uint8_t NODE_GLOSSARY_TERM    = 54;
constexpr uint8_t NODE_CROSS_REF        = 55;
constexpr uint8_t NODE_VERSION_INSERT   = 56;

// Callout types
constexpr uint8_t CALLOUT_INFO          = 0;
constexpr uint8_t CALLOUT_WARNING       = 1;
constexpr uint8_t CALLOUT_TIP           = 2;
constexpr uint8_t CALLOUT_NOTE          = 3;

// Table cell alignment
constexpr uint8_t ALIGN_NONE            = 0;
constexpr uint8_t ALIGN_LEFT            = 1;
constexpr uint8_t ALIGN_CENTER          = 2;
constexpr uint8_t ALIGN_RIGHT           = 3;

// Parser modes
constexpr uint8_t MODE_TECH             = 0;
constexpr uint8_t MODE_DSGN             = 1;

}  // namespace ase::markdown
