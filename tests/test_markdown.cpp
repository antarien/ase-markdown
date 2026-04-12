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

TEST_CASE("cmark-gfm: heading parsed correctly") {
    const char* input = "# Hello World";
    auto doc = parse(input, 13);

    CHECK(doc.root->type == NODE_DOCUMENT);
    CHECK(doc.root->first_child != nullptr);
    CHECK(doc.root->first_child->type == NODE_HEADING);
    CHECK(doc.root->first_child->heading_level == 1);

    // Heading contains text child
    auto* text = doc.root->first_child->first_child;
    CHECK(text != nullptr);
    CHECK(text->type == NODE_TEXT);
    CHECK(text->text != nullptr);
    CHECK(text->text_len == 11);

    free_document(doc);
}

TEST_CASE("cmark-gfm: heading + paragraph") {
    const char* input = "# Title\n\nSome text here.";
    auto doc = parse(input, 23);

    auto* heading = doc.root->first_child;
    CHECK(heading != nullptr);
    CHECK(heading->type == NODE_HEADING);
    CHECK(heading->heading_level == 1);

    auto* para = heading->next_sibling;
    CHECK(para != nullptr);
    CHECK(para->type == NODE_PARAGRAPH);

    auto* text = para->first_child;
    CHECK(text != nullptr);
    CHECK(text->type == NODE_TEXT);

    free_document(doc);
}

TEST_CASE("cmark-gfm: code block classified") {
    const char* input = "```cpp\nint x = 0;\n```";
    auto doc = parse(input, 21);

    auto* code = doc.root->first_child;
    CHECK(code != nullptr);
    CHECK(code->type == NODE_CODE_BLOCK);
    CHECK(code->language != nullptr);

    free_document(doc);
}

TEST_CASE("cmark-gfm: mermaid code block") {
    const char* input = "```mermaid\nflowchart TD\nA-->B\n```";
    auto doc = parse(input, 33);

    auto* block = doc.root->first_child;
    CHECK(block != nullptr);
    CHECK(block->type == NODE_MERMAID_BLOCK);

    free_document(doc);
}

TEST_CASE("cmark-gfm: diff code block") {
    const char* input = "```diff\n+ added\n- removed\n```";
    auto doc = parse(input, 29);

    auto* block = doc.root->first_child;
    CHECK(block != nullptr);
    CHECK(block->type == NODE_DIFF_BLOCK);

    free_document(doc);
}

TEST_CASE("cmark-gfm: link with url") {
    const char* input = "[click](https://example.com)";
    auto doc = parse(input, 28);

    auto* para = doc.root->first_child;
    CHECK(para != nullptr);
    auto* link = para->first_child;
    CHECK(link != nullptr);
    CHECK(link->type == NODE_LINK);
    CHECK(link->url != nullptr);

    free_document(doc);
}

TEST_CASE("cmark-gfm: ordered list") {
    const char* input = "1. First\n2. Second\n";
    auto doc = parse(input, 19);

    auto* list = doc.root->first_child;
    CHECK(list != nullptr);
    CHECK(list->type == NODE_LIST);
    CHECK(list->list_ordered == 1);
    CHECK(list->list_start == 1);

    free_document(doc);
}

TEST_CASE("cmark-gfm: emphasis and strong") {
    const char* input = "**bold** and *italic*";
    auto doc = parse(input, 21);

    auto* para = doc.root->first_child;
    CHECK(para != nullptr);
    auto* strong = para->first_child;
    CHECK(strong != nullptr);
    CHECK(strong->type == NODE_STRONG);

    free_document(doc);
}
