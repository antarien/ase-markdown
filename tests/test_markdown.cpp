#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <ase/markdown/markdown.hpp>
#include <cstring>

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

TEST_CASE("frontmatter: title and version extracted") {
    const char* input = "---\ntitle: ECS Architecture\nversion: 1.2.0\ncurated: true\n---\n# Hello";
    auto doc = parse(input, static_cast<uint32_t>(std::strlen(input)));

    CHECK(doc.frontmatter.title != nullptr);
    CHECK(std::strcmp(doc.frontmatter.title, "ECS Architecture") == 0);
    CHECK(doc.frontmatter.version != nullptr);
    CHECK(std::strcmp(doc.frontmatter.version, "1.2.0") == 0);
    CHECK(doc.frontmatter.curated == 1);

    // Body still parsed after frontmatter
    CHECK(doc.root->first_child != nullptr);
    CHECK(doc.root->first_child->type == NODE_HEADING);

    free_document(doc);
}

TEST_CASE("frontmatter: no frontmatter passes through") {
    const char* input = "# No frontmatter here";
    auto doc = parse(input, static_cast<uint32_t>(std::strlen(input)));

    CHECK(doc.frontmatter.title == nullptr);
    CHECK(doc.frontmatter.version == nullptr);
    CHECK(doc.frontmatter.curated == 0);
    CHECK(doc.root->first_child->type == NODE_HEADING);

    free_document(doc);
}

TEST_CASE("frontmatter: quoted values stripped") {
    const char* input = "---\ntitle: \"Quoted Title\"\ncategory: 'architecture'\n---\nBody";
    auto doc = parse(input, static_cast<uint32_t>(std::strlen(input)));

    CHECK(doc.frontmatter.title != nullptr);
    CHECK(std::strcmp(doc.frontmatter.title, "Quoted Title") == 0);
    CHECK(doc.frontmatter.category != nullptr);
    CHECK(std::strcmp(doc.frontmatter.category, "architecture") == 0);

    free_document(doc);
}

TEST_CASE("frontmatter: order parsed as integer") {
    const char* input = "---\norder: 42\n---\nBody";
    auto doc = parse(input, static_cast<uint32_t>(std::strlen(input)));

    CHECK(doc.frontmatter.order == 42);

    free_document(doc);
}

TEST_CASE("pass_callouts: INFO callout detected") {
    const char* input = "> [!INFO]\n> This is important.";
    auto doc = parse(input, static_cast<uint32_t>(std::strlen(input)));

    auto* callout = doc.root->first_child;
    CHECK(callout != nullptr);
    CHECK(callout->type == NODE_CALLOUT);
    CHECK(callout->callout_type == CALLOUT_INFO);

    free_document(doc);
}

TEST_CASE("pass_callouts: WARNING callout detected") {
    const char* input = "> [!WARNING]\n> Do not do this.";
    auto doc = parse(input, static_cast<uint32_t>(std::strlen(input)));

    auto* callout = doc.root->first_child;
    CHECK(callout != nullptr);
    CHECK(callout->type == NODE_CALLOUT);
    CHECK(callout->callout_type == CALLOUT_WARNING);

    free_document(doc);
}

TEST_CASE("pass_callouts: normal blockquote unchanged") {
    const char* input = "> Just a regular quote.";
    auto doc = parse(input, static_cast<uint32_t>(std::strlen(input)));

    auto* bq = doc.root->first_child;
    CHECK(bq != nullptr);
    CHECK(bq->type == NODE_BLOCKQUOTE);

    free_document(doc);
}

TEST_CASE("pass_math: inline math detected") {
    const char* input = "The formula $E=mc^2$ is famous.";
    auto doc = parse(input, static_cast<uint32_t>(std::strlen(input)));

    // Walk paragraph children looking for math node
    auto* para = doc.root->first_child;
    CHECK(para != nullptr);
    bool found_math = false;
    Node* child = para->first_child;
    while (child) {
        if (child->type == NODE_MATH_INLINE) {
            found_math = true;
            CHECK(child->text != nullptr);
            CHECK(std::strcmp(child->text, "E=mc^2") == 0);
        }
        child = child->next_sibling;
    }
    CHECK(found_math);

    free_document(doc);
}

TEST_CASE("integration: real ASE document snippet") {
    const char* input =
        "---\ntitle: Style Guide\ncurated: true\n---\n"
        "# (nf-fa-book) ASE Markdown Style Guide\n\n"
        "> (nf-fa-tag) **Ase Docs** `00.16.32` [feat]\n\n"
        "## (nf-fa-info_circle) Purpose and Scope\n\n"
        "This document defines **HOW** to format.\n\n"
        "> [!INFO]\n> The reference document is CAUSA_ASE_TIME.md.\n\n"
        "---\n\n"
        "The formula $h = g / sph$ calculates the hour.\n\n"
        "```mermaid\nflowchart TD\nA-->B\n```\n";
    auto doc = parse(input, static_cast<uint32_t>(std::strlen(input)));

    // Frontmatter
    CHECK(doc.frontmatter.title != nullptr);
    CHECK(std::strcmp(doc.frontmatter.title, "Style Guide") == 0);
    CHECK(doc.frontmatter.curated == 1);

    // Document structure
    CHECK(doc.root != nullptr);
    CHECK(doc.root->type == NODE_DOCUMENT);

    // Walk and count node types
    uint32_t headings = 0, callouts = 0, icons = 0, math = 0, mermaid = 0;
    Node* n = doc.root->first_child;
    while (n) {
        if (n->type == NODE_HEADING) headings++;
        if (n->type == NODE_CALLOUT) callouts++;
        if (n->type == NODE_MERMAID_BLOCK) mermaid++;

        // Check children for inline nodes
        Node* c = n->first_child;
        while (c) {
            if (c->type == NODE_NERDFONT_ICON) icons++;
            if (c->type == NODE_MATH_INLINE) math++;
            Node* cc = c->first_child;
            while (cc) {
                if (cc->type == NODE_NERDFONT_ICON) icons++;
                if (cc->type == NODE_MATH_INLINE) math++;
                cc = cc->next_sibling;
            }
            c = c->next_sibling;
        }
        n = n->next_sibling;
    }

    CHECK(headings == 2);    // H1 + H2
    CHECK(callouts == 1);    // [!INFO]
    CHECK(mermaid == 1);     // ```mermaid
    CHECK(icons >= 2);       // (nf-fa-book) + (nf-fa-info_circle)
    CHECK(math >= 1);        // $h = g / sph$

    free_document(doc);
}

TEST_CASE("pass_icons: NerdFont icon detected") {
    const char* input = "## (nf-fa-cube) Architecture";
    auto doc = parse(input, static_cast<uint32_t>(std::strlen(input)));

    auto* heading = doc.root->first_child;
    CHECK(heading != nullptr);
    CHECK(heading->type == NODE_HEADING);

    bool found_icon = false;
    Node* child = heading->first_child;
    while (child) {
        if (child->type == NODE_NERDFONT_ICON) {
            found_icon = true;
            CHECK(child->text != nullptr);
            CHECK(std::strcmp(child->text, "nf-fa-cube") == 0);
        }
        child = child->next_sibling;
    }
    CHECK(found_icon);

    free_document(doc);
}
