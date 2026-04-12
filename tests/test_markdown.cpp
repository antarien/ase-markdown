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
    CHECK(doc.buffer != nullptr);
    CHECK(doc.buffer_size == DOCUMENT_ARENA_SIZE);

    free_document(doc);
    CHECK(doc.root == nullptr);
    CHECK(doc.arena == nullptr);
    CHECK(doc.buffer == nullptr);
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

TEST_CASE("alloc_node creates typed node") {
    auto doc = parse("", 0);

    auto* heading = alloc_node(doc, NODE_HEADING);
    CHECK(heading != nullptr);
    CHECK(heading->type == NODE_HEADING);
    CHECK(heading->first_child == nullptr);
    CHECK(heading->next_sibling == nullptr);

    free_document(doc);
}

TEST_CASE("alloc_string duplicates into arena") {
    auto doc = parse("", 0);

    const char* original = "Hello World";
    auto* copy = alloc_string(doc, original, 11);
    CHECK(copy != nullptr);
    CHECK(copy != original);
    CHECK(copy[0] == 'H');
    CHECK(copy[10] == 'd');
    CHECK(copy[11] == '\0');

    free_document(doc);
}

TEST_CASE("append_child builds tree") {
    auto doc = parse("", 0);

    auto* child1 = alloc_node(doc, NODE_HEADING);
    auto* child2 = alloc_node(doc, NODE_PARAGRAPH);

    append_child(doc.root, child1);
    CHECK(doc.root->first_child == child1);
    CHECK(child1->parent == doc.root);

    append_child(doc.root, child2);
    CHECK(child1->next_sibling == child2);
    CHECK(child2->parent == doc.root);

    free_document(doc);
}
