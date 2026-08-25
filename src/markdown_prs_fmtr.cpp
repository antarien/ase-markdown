/*
 * ==============================================================================
 * ASE CORE INFRASTRUCTURE IMPLEMENTATION
 * ==============================================================================
 *
 * @file        markdown_prs_fmtr.cpp
 * @brief       Frontmatter pass — YAML header into the Frontmatter struct
 * @description Implements parse_frontmatter(): detects the `---\n ... \n---\n`
 *              prefix, reads the key: value pairs it recognises into the
 *              Frontmatter struct and returns the offset at which the body
 *              begins.
 *
 *              THE KEY MATCH IS A BOUNDED SCAN SINCE 2026-08-20. It read the key
 *              length with std::strlen, which the standard-library rule forbids
 *              for a reason that applies here: the function takes `const char*`
 *              and walks until it finds a NUL, however far away that is. The
 *              replacement ase::utils::str_len stops at MAX_FRONTMATTER_KEY_LEN.
 *              Every key this file passes is a literal a few characters long, so
 *              the bound never truncates a key - it removes the unbounded case,
 *              it does not cap the vocabulary.
 *
 *              fmtr = frontmatter.
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

#include <ase/markdown/frontmatter.hpp>

#include <ase/markdown/types.hpp>
#include <ase/utils/strops.hpp>

#include <cstring>

namespace ase::markdown {

namespace {

// Skip whitespace, return new offset
uint32_t skip_ws(const char* s, uint32_t pos, uint32_t len) {
    while (pos < len && (s[pos] == ' ' || s[pos] == '\t')) pos++;
    return pos;
}

// Find end of line, return offset of char after newline
uint32_t find_eol(const char* s, uint32_t pos, uint32_t len) {
    while (pos < len && s[pos] != '\n') pos++;
    return (pos < len) ? pos + 1 : pos;
}

// Extract trimmed value from line: "key: value\n" → value start+len
void extract_value(const char* line, uint32_t line_len,
                   const char*& val_start, uint32_t& val_len) {
    // Find colon
    uint32_t colon = 0;
    while (colon < line_len && line[colon] != ':') colon++;
    if (colon >= line_len) { val_start = nullptr; val_len = 0; return; }

    // Skip ": "
    uint32_t vstart = colon + 1;
    while (vstart < line_len && (line[vstart] == ' ' || line[vstart] == '\t')) vstart++;

    // Trim trailing whitespace and quotes
    uint32_t vend = line_len;
    while (vend > vstart && (line[vend - 1] == ' ' || line[vend - 1] == '\t' ||
                              line[vend - 1] == '\n' || line[vend - 1] == '\r')) vend--;

    // Strip surrounding quotes
    if (vend - vstart >= 2 && ((line[vstart] == '"' && line[vend - 1] == '"') ||
                                (line[vstart] == '\'' && line[vend - 1] == '\''))) {
        vstart++;
        vend--;
    }

    val_start = line + vstart;
    val_len = vend - vstart;
}

// Check if line starts with key (case-sensitive)
//
// The key length comes from a BOUNDED scan: ase::utils::str_len stops at
// MAX_FRONTMATTER_KEY_LEN instead of walking to the next NUL wherever that is.
// Every key passed in here is a literal of this file, the longest being
// "description" at 11 characters, so the bound never truncates a key - it only
// removes the unbounded case std::strlen would leave open.
bool starts_with_key(const char* line, uint32_t line_len, const char* key) {
    uint32_t klen = ase::utils::str_len(key, MAX_FRONTMATTER_KEY_LEN);
    if (line_len < klen + 1) return false;
    return std::memcmp(line, key, klen) == 0 && line[klen] == ':';
}

}  // anonymous namespace

uint32_t parse_frontmatter(const char* input, uint32_t len, Frontmatter& fm, Document& doc) {
    if (len < 4) return 0;

    // Must start with "---\n"
    if (input[0] != '-' || input[1] != '-' || input[2] != '-' || input[3] != '\n') return 0;

    // Find closing "---\n" or "---" at EOF
    uint32_t pos = 4;
    uint32_t body_start = 0;

    while (pos < len) {
        if (pos + 2 < len && input[pos] == '-' && input[pos + 1] == '-' && input[pos + 2] == '-') {
            // Found closing ---
            body_start = pos + 3;
            if (body_start < len && input[body_start] == '\n') body_start++;
            break;
        }
        pos = find_eol(input, pos, len);
    }

    if (body_start == 0) return 0;

    // Parse key-value pairs between the two --- markers
    pos = 4;
    while (pos < body_start - 4) {
        uint32_t line_start = pos;
        uint32_t line_end = find_eol(input, pos, body_start);
        uint32_t line_len = line_end - line_start;
        const char* line = input + line_start;

        const char* val = nullptr;
        uint32_t vlen = 0;
        extract_value(line, line_len, val, vlen);

        if (val && vlen > 0) {
            if (starts_with_key(line, line_len, "title"))
                fm.title = alloc_string(doc, val, vlen);
            else if (starts_with_key(line, line_len, "description"))
                fm.description = alloc_string(doc, val, vlen);
            else if (starts_with_key(line, line_len, "version"))
                fm.version = alloc_string(doc, val, vlen);
            else if (starts_with_key(line, line_len, "icon"))
                fm.icon = alloc_string(doc, val, vlen);
            else if (starts_with_key(line, line_len, "image"))
                fm.image = alloc_string(doc, val, vlen);
            else if (starts_with_key(line, line_len, "category"))
                fm.category = alloc_string(doc, val, vlen);
            else if (starts_with_key(line, line_len, "date"))
                fm.date = alloc_string(doc, val, vlen);
            else if (starts_with_key(line, line_len, "order")) {
                int32_t order = 0;
                for (uint32_t i = 0; i < vlen; i++) {
                    if (val[i] >= '0' && val[i] <= '9') order = order * 10 + (val[i] - '0');
                }
                fm.order = order;
            }
            else if (starts_with_key(line, line_len, "curated")) {
                fm.curated = (vlen >= 4 && val[0] == 't') ? 1 : 0;
            }
        }

        pos = line_end;
    }

    return body_start;
}

}  // namespace ase::markdown
