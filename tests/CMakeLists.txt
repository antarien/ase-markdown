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

add_test(NAME ase-markdown-test COMMAND ase-markdown-test)

set_target_properties(ase-markdown-test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
