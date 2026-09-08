cmake_minimum_required(VERSION 3.20)

find_package(doctest QUIET)
if(NOT doctest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.12
    )
    FetchContent_MakeAvailable(doctest)
endif()

add_executable(ase-markdown-test
    test_markdown.cpp
)

target_link_libraries(ase-markdown-test
    PRIVATE
        ase::markdown
        doctest::doctest
)

target_compile_features(ase-markdown-test PRIVATE cxx_std_20)

# Der doctest-Einstiegspunkt steht HIER und nicht als `#define` im Dateikopf, weil
# MACRO_DEFINITION_FORBIDDEN ihn dort meldet. Die Uebersetzungsdefinition wirkt auf der
# Kommandozeile, also VOR jedem #include — doctest erzeugt weiterhin genau ein main().
#
# PRIVATE und je ZIEL, und das ist die tragende Bedingung: die Testmuster-SSOT verlangt, dass
# GENAU EINE Uebersetzungseinheit je Executable das Makro definiert. Dieses Ziel hat GENAU EINE
# Quelle (gemessen, nicht angenommen), also trifft die Definition genau eine Einheit. Dasselbe
# gilt fuer ase-markdown-compliance weiter unten — es ist ein EIGENES Ziel mit eigener Quelle,
# nicht eine zweite Quelle desselben Ziels.
#
# WER HIER EINE ZWEITE QUELLE ANHAENGT, MUSS DIESE ZEILE ERSETZEN: bei mehreren Quellen gibt
# target_compile_definitions das Makro ALLEN — mehrere main(), Bruch am Linker. Der Weg dafuer
# ist set_source_files_properties(<die EINE Quelle> PROPERTIES COMPILE_DEFINITIONS ...).
target_compile_definitions(ase-markdown-test PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)

add_test(NAME ase-markdown-test COMMAND ase-markdown-test)

set_target_properties(ase-markdown-test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# One-off debug stat dumper for arbitrary markdown files.
add_executable(ase-markdown-debug
    debug_demo.cpp
)
target_link_libraries(ase-markdown-debug PRIVATE ase::markdown)
target_compile_features(ase-markdown-debug PRIVATE cxx_std_20)
set_target_properties(ase-markdown-debug PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Compliance tests (fixture-based)
add_executable(ase-markdown-compliance
    test_compliance.cpp
)

target_link_libraries(ase-markdown-compliance
    PRIVATE
        ase::markdown
        doctest::doctest
)

target_compile_features(ase-markdown-compliance PRIVATE cxx_std_20)

# Eigenes Ziel, eigene EINE Quelle — Begruendung und Fallstrick stehen bei ase-markdown-test.
target_compile_definitions(ase-markdown-compliance PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)

add_test(NAME ase-markdown-compliance
    COMMAND ase-markdown-compliance
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

set_target_properties(ase-markdown-compliance PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
