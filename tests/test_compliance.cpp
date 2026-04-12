#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <ase/markdown/markdown.hpp>
#include <ase/markdown/types.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

using namespace ase::markdown;

namespace {

std::string read_fixture(const char* name) {
    // Try several relative paths from potential CWD locations
    const char* dirs[] = {
        "tests/compliance/",
        "../tests/compliance/",
        "../../tests/compliance/",
        "core/ase-markdown/tests/compliance/",
        "core/core/ase-markdown/tests/compliance/"
    };
    for (const char* dir : dirs) {
        std::string path = std::string(dir) + name;
        std::ifstream f(path);
        if (f.is_open()) {
            std::ostringstream ss;
            ss << f.rdbuf();
            return ss.str();
        }
    }
    return "";
}

uint32_t count_nodes(const Node* node, uint8_t type) {
    if (!node) return 0;
    uint32_t count = (node->type == type) ? 1 : 0;
    const Node* child = node->first_child;
    while (child) {
        count += count_nodes(child, type);
        child = child->next_sibling;
    }
    return count;
}

bool has_node_type(const Node* node, uint8_t type) {
    return count_nodes(node, type) > 0;
}

}  // anonymous namespace

TEST_CASE("compliance: headings H1-H6") {
    auto input = read_fixture("test_headings.md");
    REQUIRE(!input.empty());
    auto doc = parse(input.c_str(), static_cast<uint32_t>(input.size()));
    CHECK(doc.root != nullptr);
    CHECK(count_nodes(doc.root, NODE_HEADING) == 6);
    free_document(doc);
}

TEST_CASE("compliance: callouts INFO/WARNING/TIP/NOTE") {
    auto input = read_fixture("test_callouts.md");
    REQUIRE(!input.empty());
    auto doc = parse(input.c_str(), static_cast<uint32_t>(input.size()));
    CHECK(doc.root != nullptr);
    CHECK(count_nodes(doc.root, NODE_CALLOUT) == 4);
    free_document(doc);
}

TEST_CASE("compliance: math inline and display") {
    auto input = read_fixture("test_math.md");
    REQUIRE(!input.empty());
    auto doc = parse(input.c_str(), static_cast<uint32_t>(input.size()));
    CHECK(doc.root != nullptr);
    CHECK(count_nodes(doc.root, NODE_MATH_INLINE) >= 2);
    CHECK(count_nodes(doc.root, NODE_MATH_DISPLAY) >= 1);
    free_document(doc);
}

TEST_CASE("compliance: full demo document structure") {
    auto input = read_fixture("test_full_demo.md");
    REQUIRE(!input.empty());

    ParseOptions opts{};
    opts.parse_frontmatter = 1;
    auto doc = parse(input.c_str(), static_cast<uint32_t>(input.size()), opts);

    CHECK(doc.root != nullptr);

    // Frontmatter
    CHECK(doc.frontmatter.title != nullptr);
    CHECK(std::strcmp(doc.frontmatter.title, "ASE Markdown Full Demo") == 0);
    CHECK(doc.frontmatter.curated == 1);

    // Structure counts
    CHECK(count_nodes(doc.root, NODE_HEADING) >= 6);
    CHECK(count_nodes(doc.root, NODE_PARAGRAPH) >= 4);
    CHECK(count_nodes(doc.root, NODE_CODE_BLOCK) >= 2);
    CHECK(count_nodes(doc.root, NODE_CALLOUT) == 4);
    CHECK(count_nodes(doc.root, NODE_TABLE) >= 1);
    CHECK(count_nodes(doc.root, NODE_LIST) >= 2);
    CHECK(count_nodes(doc.root, NODE_BLOCKQUOTE) >= 1);
    CHECK(count_nodes(doc.root, NODE_THEMATIC_BREAK) >= 2);

    // ASE extensions
    CHECK(has_node_type(doc.root, NODE_NERDFONT_ICON));
    CHECK(has_node_type(doc.root, NODE_MATH_INLINE));
    CHECK(has_node_type(doc.root, NODE_MERMAID_BLOCK));
    CHECK(has_node_type(doc.root, NODE_DIFF_BLOCK));
    CHECK(has_node_type(doc.root, NODE_SVGBOB_BLOCK));

    free_document(doc);
}

TEST_CASE("compliance: DSGN mode parses directives") {
    auto input = read_fixture("test_directives.md");
    REQUIRE(!input.empty());

    ParseOptions opts{};
    opts.mode = 1; // DSGN
    opts.parse_frontmatter = 1;
    auto doc = parse(input.c_str(), static_cast<uint32_t>(input.size()), opts);

    CHECK(doc.root != nullptr);

    // Directives should be present in DSGN mode
    uint32_t block_dirs = count_nodes(doc.root, NODE_BLOCK_DIRECTIVE);
    uint32_t leaf_dirs  = count_nodes(doc.root, NODE_LEAF_DIRECTIVE);

    // At minimum we should detect some block and leaf directives
    CHECK(block_dirs >= 1);
    CHECK(leaf_dirs >= 1);

    // Inline extensions
    CHECK(has_node_type(doc.root, NODE_WIKI_LINK));
    CHECK(has_node_type(doc.root, NODE_GLOSSARY_TERM));

    free_document(doc);
}
