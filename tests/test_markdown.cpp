#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <ase/markdown/markdown.hpp>

using namespace ase::markdown;

TEST_CASE("parse returns document with root node") {
    const char* input = "# Hello\n\nWorld";
    auto doc = parse(input, 14);

    CHECK(doc.root != nullptr);
    CHECK(doc.root->type == NODE_DOCUMENT);
    CHECK(doc.arena != nullptr);

    free_document(doc);
    CHECK(doc.root == nullptr);
    CHECK(doc.arena == nullptr);
}

TEST_CASE("parse empty input") {
    auto doc = parse("", 0);

    CHECK(doc.root != nullptr);
    CHECK(doc.root->type == NODE_DOCUMENT);

    free_document(doc);
}

TEST_CASE("node types are distinct") {
    CHECK(NODE_HEADING != NODE_PARAGRAPH);
    CHECK(NODE_CALLOUT != NODE_MATH_INLINE);
    CHECK(NODE_BLOCK_DIRECTIVE != NODE_LEAF_DIRECTIVE);
    CHECK(MODE_TECH == 0);
    CHECK(MODE_DSGN == 1);
}
