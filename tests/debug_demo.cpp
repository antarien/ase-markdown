/*
 * ==============================================================================
 * ASE MARKDOWN — Debug stat dump for a real document
 * ==============================================================================
 *
 * @file        debug_demo.cpp
 * @brief       One-off debug tool that parses an arbitrary markdown file
 *              (TECH or DSGN mode) and prints AST node-type counts.
 * @description Used to verify that the parser produces correct node types
 *              for the full INST_ASE_MARKDOWN_DEMO.md document — much
 *              larger than the compliance fixtures and the only way to
 *              tell whether real-world directives like nested ::tab and
 *              ::panel under :::tabs / :::accordion survive parsing.
 *
 * @module      ase-markdown
 * @layer       1 (Core)
 * @category    process/computation/algorithm
 *
 * @created     2026-04-13
 * @modified    2026-04-13
 * @version     00.00.01.00001 [seed]
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

namespace {

constexpr uint32_t MAX_TYPE = 64;

struct Counts {
    uint32_t by_type[MAX_TYPE] = {};
};

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
        std::printf("  [%s] name=%s attrs=%u children=%s\n",
                    name_for(node->type),
                    node->directive_name != nullptr ? node->directive_name : "(null)",
                    static_cast<unsigned>(node->attr_count),
                    node->first_child != nullptr ? "yes" : "no");
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <markdown_file> [tech|dsgn]\n", argv[0]);
        return 1;
    }
    const bool dsgn = (argc >= 3 && std::string{argv[2]} == "dsgn");

    std::string content;
    if (!read_file(argv[1], content)) {
        std::fprintf(stderr, "cannot read %s\n", argv[1]);
        return 1;
    }
    std::printf("file: %s (%zu bytes)\n", argv[1], content.size());
    std::printf("mode: %s\n", dsgn ? "DSGN" : "TECH");

    ase::markdown::ParseOptions opts{};
    opts.mode = dsgn ? ase::markdown::MODE_DSGN : ase::markdown::MODE_TECH;
    opts.parse_frontmatter = 1;

    auto doc = ase::markdown::parse(content.c_str(),
                                    static_cast<uint32_t>(content.size()), opts);
    if (doc.root == nullptr) {
        std::fprintf(stderr, "parse returned null root\n");
        return 1;
    }

    Counts counts;
    walk(doc.root, counts);

    std::printf("\n--- AST node counts ---\n");
    for (uint32_t t = 0; t < MAX_TYPE; ++t) {
        if (counts.by_type[t] > 0) {
            std::printf("  %-20s = %u\n", name_for(static_cast<uint8_t>(t)), counts.by_type[t]);
        }
    }

    std::printf("\n--- first 16 directives ---\n");
    int seen = 0;
    print_first_directives(doc.root, seen, 16);

    ase::markdown::free_document(doc);
    return 0;
}
