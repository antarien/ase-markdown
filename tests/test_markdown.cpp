/**
 * ASE CORE INFRASTRUCTURE IMPLEMENTATION
 *
 * @file        test_markdown.cpp
 * @brief       Drives the parser element by element: blocks, inline spans and the ASE extensions
 * @description Siebenundzwanzig Faelle an kurzen, im Code stehenden Eingaben — je ein Element und
 *              seine Randfaelle. Die Ergaenzung dazu ist test_compliance.cpp, das dieselben
 *              Elemente in vollstaendigen Vorlagendateien prueft; die Trennung ist Absicht und
 *              beide Haelften werden gebraucht.
 *
 *              WARUM BEIDE HAELFTEN: ein Fall im Code sagt genau, WELCHES Element gebrochen ist,
 *              und ist beim Fehlschlag in einer Zeile zu lesen. Er kann aber nur die Umgebung
 *              pruefen, an die der Schreiber gedacht hat. Die Vorlagendateien liefern die
 *              Umgebung, an die niemand gedacht hat, sagen beim Fehlschlag aber nur, dass eine
 *              Zahl nicht stimmt. Faellt eine der beiden Haelften weg, verliert der Parser
 *              entweder seine Diagnose oder seine Abdeckung.
 *
 *              DIE EINGABELAENGE WIRD GERECHNET, NICHT GEZAEHLT, und der Grund steht
 *              ausgeschrieben beim ersten Fall: `const char input[]` plus sizeof(input) - 1 macht
 *              die Laenge zu einer Uebersetzungszeit-Tatsache. Eine von Hand gezaehlte Zahl
 *              veraltet still, sobald jemand die Zeichenkette darueber aendert — der Test prueft
 *              dann einen abgeschnittenen Text weiter, ohne rot zu werden. Diese Form ist der
 *              Grund, warum die Datei ohne std::strlen auskommt.
 *
 * @module      ase-markdown
 * @layer       1 (Core)
 * @category    process/validation/check
 * @usecase     usecase:100008 - render the knowledge stock
 * @created     2026-09-01
 * @modified    2026-10-13
 * @version     1.1.0
 *
 * CORE INFRASTRUCTURE IMPLEMENTATION COMPLIANCE
 *
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
 */

#include <doctest/doctest.h>
#include <ase/markdown/markdown.hpp>
#include <cstring>

/**
 * DIE FAELLE STEHEN IM NAMENSRAUM DES GEPRUEFTEN, nicht hinter einem `using namespace` auf
 * Dateiebene: das zieht JEDEN Namen des Moduls dorthin, wo auch doctest und die
 * Standardbibliothek stehen, und eine kuenftige Namensgleichheit traefe eine Datei, die mit der
 * Sache nichts zu tun hat.
 *
 * Der Namensraum umschliesst die ganze Datei. Sie hat keine eigenen Helfer — es gaebe nichts,
 * was in einen engeren Block gehoerte, und ein leerer Namensraum neben den Faellen waere die
 * Form ohne die Sache.
 */
namespace ase::markdown {

TEST_CASE("parse returns document with root node") {
    const char input[] = "# Hello\n\nWorld";
    /* DIE LAENGE WIRD GERECHNET, NICHT GEZAEHLT — und das behebt zwei Dinge auf einmal.
     *
     * Eine von Hand gezaehlte Zahl ist die gefaehrlichere der beiden naheliegenden Formen: wer
     * die Zeichenkette darueber aendert, laesst die Zahl still veralten, und der Test prueft
     * danach einen abgeschnittenen Text weiter, ohne rot zu werden. std::strlen(input) ist die
     * zweite, und die meldet der Validator (C_STRING_FUNCTIONS_FORBIDDEN).
     *
     * `const char input[]` statt `const char*` macht die Laenge zu einer Uebersetzungszeit-
     * Tatsache. sizeof(input) - 1 ist exakt, kann nicht veralten und braucht weder strlen noch
     * einen Helfer. Der Abzug von eins ist der Nullterminator, den parse() nicht zaehlt. */
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

    CHECK(doc.root != nullptr);
    CHECK(doc.root->type == NODE_DOCUMENT);
    CHECK(doc.arena != nullptr);
    CHECK(doc.buffer != nullptr);
    /* DIE ARENA IST SO GROSS, WIE DIE EINGABE ES VERLANGT, nicht so gross wie eine Konstante.
     * Geprueft wird deshalb gegen dieselbe Rechnung, die parse() fuehrt: eine hier eingetragene
     * Zahl waere eine zweite Quelle fuer denselben Wert. */
    CHECK(doc.buffer_size == document_arena_size(static_cast<uint32_t>(sizeof(input) - 1)));
    CHECK(doc.arena_exhausted == 0);

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

TEST_CASE("arena size follows the input and stops at the ceiling") {
    /* Die drei Konstanten tragen die Rechnung, nicht der Test: er prueft die FORM der Rechnung
     * (Boden, Vielfaches, Deckel), damit eine spaetere Anpassung der Werte hier nichts bricht,
     * eine Anpassung der Rechnung aber schon. */
    CHECK(document_arena_size(0) == DOCUMENT_ARENA_FLOOR);
    CHECK(document_arena_size(1000) == 1000 * DOCUMENT_ARENA_FACTOR + DOCUMENT_ARENA_FLOOR);
    CHECK(document_arena_size(DOCUMENT_ARENA_CEILING) == DOCUMENT_ARENA_CEILING);

    CHECK(document_arena_grown(DOCUMENT_ARENA_FLOOR) == 2 * DOCUMENT_ARENA_FLOOR);
    CHECK(document_arena_grown(DOCUMENT_ARENA_CEILING / 2) == DOCUMENT_ARENA_CEILING);
    CHECK(document_arena_grown(DOCUMENT_ARENA_CEILING) == DOCUMENT_ARENA_CEILING);
}

TEST_CASE("a full arena ends a pass instead of crashing it") {
    /* WARUM DIE ARENA HIER ABSICHTLICH VOLL IST: ein Pass, der aus einer vollen Arena nullptr
     * bekommt und hindurchschreibt, stirbt mitten im Oeffnen einer Datei — mit SIGSEGV, ohne
     * Meldung, an einer Stelle, die wie ein Darstellungsfehler aussieht. Dieser Fall laeuft genau
     * diesen Zweig. Die Eingabe bleibt winzig, weil nicht ihre GROESSE den Fall macht, sondern
     * der leere Rest der Arena. */
    const char input[] = "plain text";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));
    REQUIRE(doc.root != nullptr);
    CHECK(doc.arena_exhausted == 0);

    // Ein Textknoten, der ein Symbolmuster traegt und noch ungeteilt ist.
    auto* para = alloc_node(doc, NODE_PARAGRAPH);
    auto* txt  = alloc_node(doc, NODE_TEXT);
    REQUIRE(para != nullptr);
    REQUIRE(txt != nullptr);
    const char payload[] = "(nf-fa-cubes) tail";
    txt->text = alloc_string(doc, payload, static_cast<uint32_t>(sizeof(payload) - 1));
    REQUIRE(txt->text != nullptr);
    txt->text_len = static_cast<uint32_t>(sizeof(payload) - 1);
    append_child(para, txt);
    append_child(doc.root, para);

    // Jeder weitere Knoten nimmt Platz, bis keiner mehr hineingeht.
    uint32_t filled = 0;
    while (alloc_node(doc, NODE_TEXT) != nullptr) filled++;
    CHECK(filled > 0);                 // die Fuellung hat wirklich stattgefunden
    CHECK(doc.arena_exhausted == 1);

    doc.arena_exhausted = 0;           // ab hier zaehlt nur, was der Pass selbst meldet
    pass_icons(doc);

    CHECK(doc.arena_exhausted == 1);   // der Pass MELDET den vollen Zustand
    CHECK(txt->type == NODE_TEXT);     // und laesst den Knoten unberuehrt
    CHECK(txt->text_len == static_cast<uint32_t>(sizeof(payload) - 1));

    free_document(doc);
}

TEST_CASE("a document past the first arena guess still parses whole") {
    /* DER TEUERSTE DOKUMENTTYP IST DER TABELLENSCHWERE, nicht der laengste: jede Zelle ist ein
     * eigener Knoten mit eigenem Textstueck, und jedes Symbol darin teilt dieses Stueck noch
     * einmal. Die Eingabe hier wird deshalb aus Tabellenzeilen MIT Symbol gebaut, nicht aus
     * Prosa — sie steht fuer die Dokumente, an denen eine feste Arena zuerst zerbricht.
     *
     * Erzeugt statt kopiert: eine echte Datei als Vorlage hereinzuholen waere eine zweite
     * Fassung desselben Textes an einem zweiten Ort. */
    constexpr uint32_t Capacity = 262144;
    char big[Capacity];
    uint32_t len = 0;

    auto put = [&](const char* text, uint32_t count) {
        for (uint32_t i = 0; i < count && len < Capacity; ++i) big[len++] = text[i];
    };

    const char head[] = "# (nf-fa-cubes) Big\n\n| a | b | c |\n| - | - | - |\n";
    const char row[]  = "| (nf-fa-tag) alpha | beta gamma | delta epsilon |\n";
    put(head, static_cast<uint32_t>(sizeof(head) - 1));
    while (len + static_cast<uint32_t>(sizeof(row) - 1) < Capacity) {
        put(row, static_cast<uint32_t>(sizeof(row) - 1));
    }

    auto doc = parse(big, len);
    MESSAGE("input " << len << " bytes, arena " << doc.buffer_size
                     << " bytes, first guess " << document_arena_size(len));

    CHECK(doc.root != nullptr);
    CHECK(doc.arena_exhausted == 0);                          // der Baum ist VOLLSTAENDIG
    CHECK(doc.buffer_size >= document_arena_size(len));       // gewachsen, falls die Schaetzung zu klein war
    CHECK(doc.buffer_size <= DOCUMENT_ARENA_CEILING);

    // Der Knoten, an dessen Anlage eine volle Arena scheitert, steht in der Ueberschrift.
    auto* heading = doc.root->first_child;
    REQUIRE(heading != nullptr);
    bool found_icon = false;
    Node* child = heading->first_child;
    while (child) {
        if (child->type == NODE_NERDFONT_ICON) found_icon = true;
        child = child->next_sibling;
    }
    CHECK(found_icon);

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
    const char input[] = "# Hello World";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

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
    const char input[] = "# Title\n\nSome text here.";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

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
    const char input[] = "```cpp\nint x = 0;\n```";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

    auto* code = doc.root->first_child;
    CHECK(code != nullptr);
    CHECK(code->type == NODE_CODE_BLOCK);
    CHECK(code->language != nullptr);

    free_document(doc);
}

TEST_CASE("cmark-gfm: mermaid code block") {
    const char input[] = "```mermaid\nflowchart TD\nA-->B\n```";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

    auto* block = doc.root->first_child;
    CHECK(block != nullptr);
    CHECK(block->type == NODE_MERMAID_BLOCK);

    free_document(doc);
}

TEST_CASE("cmark-gfm: diff code block") {
    const char input[] = "```diff\n+ added\n- removed\n```";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

    auto* block = doc.root->first_child;
    CHECK(block != nullptr);
    CHECK(block->type == NODE_DIFF_BLOCK);

    free_document(doc);
}

TEST_CASE("cmark-gfm: link with url") {
    const char input[] = "[click](https://example.com)";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

    auto* para = doc.root->first_child;
    CHECK(para != nullptr);
    auto* link = para->first_child;
    CHECK(link != nullptr);
    CHECK(link->type == NODE_LINK);
    CHECK(link->url != nullptr);

    free_document(doc);
}

TEST_CASE("cmark-gfm: ordered list") {
    const char input[] = "1. First\n2. Second\n";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

    auto* list = doc.root->first_child;
    CHECK(list != nullptr);
    CHECK(list->type == NODE_LIST);
    CHECK(list->list_ordered == 1);
    CHECK(list->list_start == 1);

    free_document(doc);
}

TEST_CASE("cmark-gfm: emphasis and strong") {
    const char input[] = "**bold** and *italic*";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

    auto* para = doc.root->first_child;
    CHECK(para != nullptr);
    auto* strong = para->first_child;
    CHECK(strong != nullptr);
    CHECK(strong->type == NODE_STRONG);

    free_document(doc);
}

TEST_CASE("frontmatter: title and version extracted") {
    const char input[] = "---\ntitle: ECS Architecture\nversion: 1.2.0\ncurated: true\n---\n# Hello";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

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
    const char input[] = "# No frontmatter here";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

    CHECK(doc.frontmatter.title == nullptr);
    CHECK(doc.frontmatter.version == nullptr);
    CHECK(doc.frontmatter.curated == 0);
    CHECK(doc.root->first_child->type == NODE_HEADING);

    free_document(doc);
}

TEST_CASE("frontmatter: quoted values stripped") {
    const char input[] = "---\ntitle: \"Quoted Title\"\ncategory: 'architecture'\n---\nBody";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

    CHECK(doc.frontmatter.title != nullptr);
    CHECK(std::strcmp(doc.frontmatter.title, "Quoted Title") == 0);
    CHECK(doc.frontmatter.category != nullptr);
    CHECK(std::strcmp(doc.frontmatter.category, "architecture") == 0);

    free_document(doc);
}

TEST_CASE("frontmatter: order parsed as integer") {
    const char input[] = "---\norder: 42\n---\nBody";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

    CHECK(doc.frontmatter.order == 42);

    free_document(doc);
}

TEST_CASE("pass_callouts: INFO callout detected") {
    const char input[] = "> [!INFO]\n> This is important.";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

    auto* callout = doc.root->first_child;
    CHECK(callout != nullptr);
    CHECK(callout->type == NODE_CALLOUT);
    CHECK(callout->callout_type == CALLOUT_INFO);

    free_document(doc);
}

TEST_CASE("pass_callouts: WARNING callout detected") {
    const char input[] = "> [!WARNING]\n> Do not do this.";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

    auto* callout = doc.root->first_child;
    CHECK(callout != nullptr);
    CHECK(callout->type == NODE_CALLOUT);
    CHECK(callout->callout_type == CALLOUT_WARNING);

    free_document(doc);
}

TEST_CASE("pass_callouts: normal blockquote unchanged") {
    const char input[] = "> Just a regular quote.";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

    auto* bq = doc.root->first_child;
    CHECK(bq != nullptr);
    CHECK(bq->type == NODE_BLOCKQUOTE);

    free_document(doc);
}

TEST_CASE("pass_math: inline math detected") {
    const char input[] = "The formula $E=mc^2$ is famous.";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

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
    const char input[] =
        "---\ntitle: Style Guide\ncurated: true\n---\n"
        "# (nf-fa-book) ASE Markdown Style Guide\n\n"
        "> (nf-fa-tag) **Ase Docs** `00.16.32` [feat]\n\n"
        "## (nf-fa-info_circle) Purpose and Scope\n\n"
        "This document defines **HOW** to format.\n\n"
        "> [!INFO]\n> The reference document is CAUSA_ASE_TIME.md.\n\n"
        "---\n\n"
        "The formula $h = g / sph$ calculates the hour.\n\n"
        "```mermaid\nflowchart TD\nA-->B\n```\n";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

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
    const char input[] = "## (nf-fa-cube) Architecture";
    auto doc = parse(input, static_cast<uint32_t>(sizeof(input) - 1));

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

}  // namespace ase::markdown
