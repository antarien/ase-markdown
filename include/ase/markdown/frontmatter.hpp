#pragma once

/**
 * Frontmatter parser — extracts YAML header from markdown input.
 *
 * Detects `---\n...\n---\n` prefix, parses key: value pairs,
 * populates Frontmatter struct, returns remaining body offset.
 */

#include <ase/markdown/ast.hpp>
#include <cstdint>

namespace ase::markdown {

// Parse frontmatter from input. Returns byte offset where body starts.
// If no frontmatter found, returns 0 and leaves fm unchanged.
uint32_t parse_frontmatter(const char* input, uint32_t len, Frontmatter& fm, Document& doc);

}  // namespace ase::markdown
