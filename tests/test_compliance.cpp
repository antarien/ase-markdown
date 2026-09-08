/**
 * ASE CORE INFRASTRUCTURE IMPLEMENTATION
 *
 * @file        test_compliance.cpp
 * @brief       Parses five real Markdown fixtures and counts the node types they must produce
 * @description Fuenf Faelle, die den Parser NICHT an handgeschriebenen Schnipseln pruefen,
 *              sondern an vollstaendigen Vorlagendateien unter tests/compliance/ — Ueberschriften,
 *              Hinweiskaesten, Mathematik, ein volles Demodokument und der DSGN-Modus mit
 *              Direktiven.
 *
 *              WARUM AN VORLAGENDATEIEN UND NICHT AN ZEICHENKETTEN IM CODE: ein Parser scheitert
 *              selten am einzelnen Element, sondern an seiner UMGEBUNG — eine Ueberschrift direkt
 *              nach einem Codeblock, ein Hinweiskasten in einer Liste. Ein Schnipsel im Testcode
 *              enthaelt genau die Umgebung, an die der Schreiber gedacht hat; eine echte Datei
 *              enthaelt die, an die niemand gedacht hat.
 *
 *              WARUM GEZAEHLT UND NICHT NUR AUF VORHANDENSEIN GEPRUEFT: ein Parser, der ein
 *              Element DOPPELT erzeugt, besteht jede Vorhandenseinspruefung. Die Faelle mit
 *              fester Zahl (sechs Ueberschriften, vier Hinweiskaesten) fangen genau das; wo die
 *              Vorlage waechst, steht bewusst `>=`, damit eine ERGAENZTE Vorlage den Test nicht
 *              rot macht, ohne dass am Parser etwas falsch ist.
 *
 *              read_fixture SUCHT MEHRERE VERZEICHNISSE AB, weil das Arbeitsverzeichnis eines
 *              Testlaufs nicht festliegt. Es urteilt ueber INHALT, nicht ueber Oeffnen-Koennen:
 *              eine leere Vorlagendatei traegt null erwartete Knoten, der Fall faellt so oder so,
 *              und die Suche laeuft richtigerweise zum naechsten Verzeichnis weiter. Jeder Fall
 *              beginnt mit REQUIRE auf nicht-leer — sonst pruefte er den Parser an nichts und
 *              waere gruen.
 *
 * @module      ase-markdown
 * @layer       1 (Core)
 * @category    process/validation/check
 * @created     2026-09-01
 * @modified    2026-09-01
 * @version     1.0.0
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
#include <ase/markdown/types.hpp>
#include <ase/fileio/text_reader.hpp>
#include <string>
#include <cstring>

/**
 * DIE FAELLE STEHEN IM NAMENSRAUM DES GEPRUEFTEN. Hier stand `using namespace ase::markdown;` auf
 * Dateiebene: das zieht JEDEN Namen des Moduls dorthin, wo auch doctest und die
 * Standardbibliothek stehen.
 *
 * Der Namensraum umschliesst die GANZE Datei, nicht nur den Helferblock. Die drei Helfer nehmen
 * `const Node*` und `const char*`; beim ersten liegt der Argumenttyp im Modul, beim zweiten
 * nicht. Eine Anordnung, die den Namensraum nur um die Helfer legt und die Faelle draussen
 * laesst, haengt damit an der argumentabhaengigen Namenssuche — sie traegt fuer count_nodes und
 * has_node_type und NICHT fuer read_fixture. Der volle Einschluss braucht diesen Zufall nicht.
 */
namespace ase::markdown {

namespace {

std::string read_fixture(const char* name) {
    // Try several relative paths from potential CWD locations
    const char* dirs[] = {
        "tests/compliance/",
        "../tests/compliance/",
        "../../tests/compliance/",
        "core/ase-markdown/tests/compliance/"
    };
    /* ase::fileio::read_text statt Strom plus Stringstream — und der Umbau macht die Funktion
     * zugleich kuerzer, nicht nur konform.
     *
     * Die alte Fassung brauchte DREI Schritte fuer eine Frage: Strom oeffnen, is_open pruefen,
     * ueber einen Stringstream in eine Zeichenkette umschaufeln. read_text beantwortet sie in
     * einem und gibt bei jedem Lesefehler eine LEERE Zeichenkette zurueck.
     *
     * DIE PRUEFUNG WECHSELT DAMIT VON "liess sich oeffnen" AUF "hat Inhalt", und fuer diesen
     * Aufrufer ist das dasselbe Urteil: eine Vorlagendatei, die es gibt und die leer ist,
     * traegt genau null erwartete Knoten — der Test darauf faellt so oder so. Der einzige
     * Unterschied waere ein Verzeichnis, in dem die Datei leer existiert, waehrend ein
     * spaeteres sie gefuellt haette; dann faehrt die Schleife jetzt richtigerweise weiter. */
    for (const char* dir : dirs) {
        const std::string path = std::string(dir) + name;
        std::string inhalt = ase::fileio::read_text(path);
        if (!inhalt.empty()) {
            return inhalt;
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

}  // namespace

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

}  // namespace ase::markdown
