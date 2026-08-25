#pragma once

/*
 * ==============================================================================
 * ASE CORE INFRASTRUCTURE HEADER
 * ==============================================================================
 *
 * @file        frontmatter.hpp
 * @brief       The YAML frontmatter pass of ase-markdown
 * @description Detects the `---\n ... \n---\n` prefix of a markdown input, parses
 *              its key: value pairs into the Frontmatter struct and reports the
 *              offset at which the body begins.
 *
 *              THE OFFSET IS THE POINT. The pass does not consume or copy the
 *              input: it returns where the body starts, so the block parser reads
 *              the same buffer from there. A frontmatter that stripped its header
 *              into a second buffer would double the input in memory and cost
 *              every later position report its meaning - reported offsets would
 *              no longer address the caller's own text.
 *
 * @module      ase-markdown
 * @layer       1 (Core)
 * @category    process/computation/algorithm
 *
 * @created     2026-04-12
 * @modified    2026-08-20
 * @version     1.0.0
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

#include <ase/markdown/ast.hpp>
#include <cstdint>

namespace ase::markdown {

// Parse frontmatter from input. Returns byte offset where body starts.
// If no frontmatter found, returns 0 and leaves fm unchanged.
uint32_t parse_frontmatter(const char* input, uint32_t len, Frontmatter& fm, Document& doc);

}  // namespace ase::markdown
