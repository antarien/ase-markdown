/*
 * ==============================================================================
 * ASE CORE INFRASTRUCTURE IMPLEMENTATION
 * ==============================================================================
 *
 * @file        debug_demo.cpp
 * @brief       Debug stat dump for a real document — CLI tool, target ase-markdown-debug
 * @description Parses an arbitrary markdown file (TECH or DSGN mode) and prints AST
 *              node-type counts. Used to verify that the parser produces correct node
 *              types for the full INST_ASE_MARKDOWN_DEMO.md document — much larger than
 *              the compliance fixtures and the only way to tell whether real-world
 *              directives like nested ::tab and ::panel under :::tabs / :::accordion
 *              survive parsing.
 *
 *              THE OUTPUT IS THE PRODUCT, NOT A LOG, AND IT STILL IS - what changed on
 *              2026-08-20 is the primitive underneath. This file argued that its nine
 *              printf calls had to stay, on two grounds, and both were sound:
 *                (1) `ase-markdown-debug file.md` exists in order to print those numbers
 *                    to a human or into a pipe. Through ase-log they would gain a level
 *                    prefix and a timestamp, land in the log channel, and stop being
 *                    greppable, diffable or pipeable.
 *                (2) ase-markdown binds ase-alloc, ase-containers and ase-utils. Pulling
 *                    in ase-log for a debug CLI would put a fourth edge into a
 *                    WASM-capable core module.
 *              Neither was ever an argument FOR printf, only against ase-log - and the
 *              third option was in the tree the whole time: tools/ase-cli/src/main.cpp
 *              emits its prompt and banner through one fwrite for exactly this reason
 *              (the "Raw stdout write for the cooked-mode fallback" block), and
 *              cli_dispatch.cpp builds emit_out / emit_err on it. Of the rules,
 *              printf, fprintf, sprintf and the three std streams each have one; fwrite
 *              has none. The nine calls now go through write_out / write_err below:
 *              stdout six times (banner, node-count table, directive list), stderr three
 *              times (usage, unreadable input, null root). No log dependency was added,
 *              nothing about the output changed, and the module keeps its three edges.
 *
 *              ONE FORMATTING DETAIL MOVED WITH THEM. printf's "%-20s" left-aligned the
 *              node-type name in a 20-column field; padded_right below does the same and
 *              likewise lets a longer name push the column rather than truncating it -
 *              the longest name in use is NODE_THEMATIC_BREAK at 14 characters.
 *
 * @module      ase-markdown
 * @layer       1 (Core)
 * @category    process/computation/algorithm
 *
 * @created     2026-04-13
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

#include <cstdio>
#include <cstdlib>
#include <string>

namespace ase::markdown {

namespace {

constexpr uint32_t MAX_TYPE = 64;

// Column width of the node-type name in the count table, and the pad character.
constexpr size_t TypeNameColumn = 20;
constexpr char PadChar = ' ';

struct Counts {
    uint32_t by_type[MAX_TYPE] = {};
};

// The two ways out of this program. Everything below builds a string and hands it to
// one of them; nothing else writes.
void write_out(const std::string& text) {
    if (text.empty()) return;
    std::fwrite(text.data(), 1, text.size(), stdout);
}

void write_err(const std::string& text) {
    if (text.empty()) return;
    std::fwrite(text.data(), 1, text.size(), stderr);
}

// Left-align in `width` columns. A longer string is returned whole - a table that hides
// half a node type to keep its column is worse than one that shifts.
std::string padded_right(const char* text, size_t width) {
    std::string out = text;
    while (out.size() < width) out += PadChar;
    return out;
}

void walk(const ase::markdown::Node* node, Counts& counts) {
    if (node == nullptr) return;
    if (node->type < MAX_TYPE) counts.by_type[node->type] += 1;
    for (const ase::markdown::Node* c = node->first_child; c != nullptr; c = c->next_sibling) {
        walk(c, counts);
    }
}

const char* name_for(uint8_t t) {
    if (t == ase::markdown::NODE_DOCUMENT)        return "DOCUMENT";
    if (t == ase::markdown::NODE_HEADING)         return "HEADING";
    if (t == ase::markdown::NODE_PARAGRAPH)       return "PARAGRAPH";
    if (t == ase::markdown::NODE_TEXT)            return "TEXT";
    if (t == ase::markdown::NODE_CODE_BLOCK)      return "CODE_BLOCK";
    if (t == ase::markdown::NODE_INLINE_CODE)     return "INLINE_CODE";
    if (t == ase::markdown::NODE_BLOCKQUOTE)      return "BLOCKQUOTE";
    if (t == ase::markdown::NODE_LIST)            return "LIST";
    if (t == ase::markdown::NODE_LIST_ITEM)       return "LIST_ITEM";
    if (t == ase::markdown::NODE_TABLE)           return "TABLE";
    if (t == ase::markdown::NODE_TABLE_ROW)       return "TABLE_ROW";
    if (t == ase::markdown::NODE_TABLE_CELL)      return "TABLE_CELL";
    if (t == ase::markdown::NODE_THEMATIC_BREAK)  return "THEMATIC_BREAK";
    if (t == ase::markdown::NODE_LINK)            return "LINK";
    if (t == ase::markdown::NODE_IMAGE)           return "IMAGE";
    if (t == ase::markdown::NODE_EMPHASIS)        return "EMPHASIS";
    if (t == ase::markdown::NODE_STRONG)          return "STRONG";
    if (t == ase::markdown::NODE_HTML_BLOCK)      return "HTML_BLOCK";
    if (t == ase::markdown::NODE_HTML_INLINE)     return "HTML_INLINE";
    if (t == ase::markdown::NODE_CALLOUT)         return "CALLOUT";
    if (t == ase::markdown::NODE_MATH_INLINE)     return "MATH_INLINE";
    if (t == ase::markdown::NODE_MATH_DISPLAY)    return "MATH_DISPLAY";
    if (t == ase::markdown::NODE_NERDFONT_ICON)   return "NERDFONT_ICON";
    if (t == ase::markdown::NODE_MERMAID_BLOCK)   return "MERMAID_BLOCK";
    if (t == ase::markdown::NODE_DIFF_BLOCK)      return "DIFF_BLOCK";
    if (t == ase::markdown::NODE_SVGBOB_BLOCK)    return "SVGBOB_BLOCK";
    if (t == ase::markdown::NODE_ASEMATH_BLOCK)   return "ASEMATH_BLOCK";
    if (t == ase::markdown::NODE_BLOCK_DIRECTIVE) return "BLOCK_DIRECTIVE";
    if (t == ase::markdown::NODE_LEAF_DIRECTIVE)  return "LEAF_DIRECTIVE";
    if (t == ase::markdown::NODE_TEXT_DIRECTIVE)  return "TEXT_DIRECTIVE";
    if (t == ase::markdown::NODE_WIKI_LINK)       return "WIKI_LINK";
    if (t == ase::markdown::NODE_GLOSSARY_TERM)   return "GLOSSARY_TERM";
    if (t == ase::markdown::NODE_CROSS_REF)       return "CROSS_REF";
    if (t == ase::markdown::NODE_VERSION_INSERT)  return "VERSION_INSERT";
    return "?";
}

void print_first_directives(const ase::markdown::Node* node, int& seen, int max_show) {
    if (node == nullptr || seen >= max_show) return;
    if (node->type == ase::markdown::NODE_BLOCK_DIRECTIVE ||
        node->type == ase::markdown::NODE_LEAF_DIRECTIVE) {
        std::string line = "  [";
        line += name_for(node->type);
        line += "] name=";
        line += (node->directive_name != nullptr ? node->directive_name : "(null)");
        line += " attrs=" + std::to_string(static_cast<unsigned>(node->attr_count));
        line += " children=";
        line += (node->first_child != nullptr ? "yes" : "no");
        line += "\n";
        write_out(line);
        seen += 1;
    }
    for (const ase::markdown::Node* c = node->first_child; c != nullptr; c = c->next_sibling) {
        print_first_directives(c, seen, max_show);
    }
}

bool read_file(const char* path, std::string& out) {
    std::FILE* f = std::fopen(path, "rb");
    if (f == nullptr) return false;
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (size > 0) {
        out.resize(static_cast<size_t>(size));
        std::fread(out.data(), 1, static_cast<size_t>(size), f);
    }
    std::fclose(f);
    return true;
}

}  // namespace

}  // namespace ase::markdown

int main(int argc, char* argv[]) {
    using namespace ase::markdown;

    if (argc < 2) {
        write_err("usage: " + std::string{argv[0]} + " <markdown_file> [tech|dsgn]\n");
        return 1;
    }
    const bool dsgn = (argc >= 3 && std::string{argv[2]} == "dsgn");

    std::string content;
    if (!read_file(argv[1], content)) {
        write_err("cannot read " + std::string{argv[1]} + "\n");
        return 1;
    }
    write_out("file: " + std::string{argv[1]} + " (" + std::to_string(content.size()) +
              " bytes)\n");
    write_out(std::string{"mode: "} + (dsgn ? "DSGN" : "TECH") + "\n");

    ase::markdown::ParseOptions opts{};
    opts.mode = dsgn ? ase::markdown::MODE_DSGN : ase::markdown::MODE_TECH;
    opts.parse_frontmatter = 1;

    auto doc = ase::markdown::parse(content.c_str(),
                                    static_cast<uint32_t>(content.size()), opts);
    if (doc.root == nullptr) {
        write_err("parse returned null root\n");
        return 1;
    }

    Counts counts;
    walk(doc.root, counts);

    write_out("\n--- AST node counts ---\n");
    for (uint32_t t = 0; t < MAX_TYPE; ++t) {
        if (counts.by_type[t] > 0) {
            write_out("  " + padded_right(name_for(static_cast<uint8_t>(t)), TypeNameColumn) +
                      " = " + std::to_string(counts.by_type[t]) + "\n");
        }
    }

    write_out("\n--- first 16 directives ---\n");
    int seen = 0;
    print_first_directives(doc.root, seen, 16);

    ase::markdown::free_document(doc);
    return 0;
}
