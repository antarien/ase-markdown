#pragma once

/*
 * ==============================================================================
 * ASE CORE INFRASTRUCTURE HEADER
 * ==============================================================================
 *
 * @file        directives.hpp
 * @brief       The directive NAME registry of the CMS DSL (DSGN mode)
 * @description The single place where the directive names of the DSL are written
 *              down: 21 block directives (:::name{attrs} ... :::), 10 leaf
 *              directives (::name{attrs}) and 7 inline extensions ([[wiki]],
 *              {{glossary}}, {ref:}, {icon:}, {version}, :tip[]{}).
 *
 *              WHY A REGISTRY AND NOT STRING LITERALS AT THE CALL SITES: the DSL
 *              pass writes these names into Node::directive_name and every
 *              consumer dispatches on them. Repeated literals would put the same
 *              spelling in two places with nothing comparing them - a renamed
 *              directive would parse on one side and silently never match on the
 *              other. The umbrella markdown.hpp re-exports this header for that
 *              reason.
 *
 * @module      ase-markdown
 * @layer       1 (Core)
 * @category    process/computation/algorithm
 *
 * @created     2026-04-13
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

namespace ase::markdown::directive {

// ── Block Directives (:::name{attrs} ... :::) ──────────────────────

constexpr const char* HERO        = "hero";
constexpr const char* COLUMNS     = "columns";
constexpr const char* CARDS       = "cards";
constexpr const char* TABS        = "tabs";
constexpr const char* ACCORDION   = "accordion";
constexpr const char* TIMELINE    = "timeline";
constexpr const char* STATS       = "stats";
constexpr const char* STEPS       = "steps";
constexpr const char* COMPARE     = "compare";
constexpr const char* TEAM        = "team";
constexpr const char* CHANGELOG   = "changelog";
constexpr const char* MATRIX      = "matrix";
constexpr const char* TERMINAL    = "terminal";
constexpr const char* CODE        = "code";
constexpr const char* CALLOUT     = "callout";
constexpr const char* FIGURE      = "figure";
constexpr const char* GALLERY     = "gallery";
constexpr const char* ASIDE       = "aside";
constexpr const char* QUOTE       = "quote";
constexpr const char* AUTHOR      = "author";
constexpr const char* TOC         = "toc";

// ── Child Directives (::child inside block) ────────────────────────

constexpr const char* CARD        = "card";
constexpr const char* TAB         = "tab";
constexpr const char* PANEL       = "panel";
constexpr const char* EVENT       = "event";
constexpr const char* STAT        = "stat";
constexpr const char* STEP        = "step";
constexpr const char* OPTION      = "option";
constexpr const char* MEMBER      = "member";
constexpr const char* ROW         = "row";
constexpr const char* COL         = "col";

// ── Leaf Directives (::name{attrs}) ────────────────────────────────

constexpr const char* BADGE       = "badge";
constexpr const char* DIVIDER     = "divider";
constexpr const char* SPACER      = "spacer";
constexpr const char* PROGRESS    = "progress";
constexpr const char* VIDEO       = "video";
constexpr const char* AUDIO       = "audio";
constexpr const char* DOWNLOAD    = "download";
constexpr const char* EMBED       = "embed";
constexpr const char* PREVIEW     = "preview";
constexpr const char* KBD         = "kbd";

// ── Inline Extensions ──────────────────────────────────────────────
// These are parsed by pass_inline_extensions() from text content:
// [[page]]        → NODE_WIKI_LINK
// {{term}}        → NODE_GLOSSARY_TERM
// {ref:path|text} → NODE_CROSS_REF
// {icon:nf-name}  → NODE_NERDFONT_ICON (handled by pass_icons)
// {version}       → NODE_VERSION_INSERT
// :tip[text]{tip} → NODE_TEXT_DIRECTIVE with directive_name="tip"
constexpr const char* TIP         = "tip";

}  // namespace ase::markdown::directive
