#pragma once

/**
 * Directive name constants for CMS DSL (DSGN mode).
 *
 * 21 Block Directives (:::name{attrs} ... :::)
 * 10 Leaf Directives  (::name{attrs})
 *  7 Inline Extensions ([[wiki]], {{glossary}}, {ref:}, {icon:}, {version}, :tip[]{})
 *
 * @module   ase-markdown
 * @layer    1 (Core)
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
