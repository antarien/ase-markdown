cmake_minimum_required(VERSION 3.20)

#[[
ASE CORE BUILD CONFIGURATION

@file        CMakeLists.txt
@brief       Build configuration for ase-markdown
@description CommonMark/GFM + ASE DSL Markdown parser producing typed AST.

@module      ase-markdown
@layer       1 (Core)
@created     2026-04-12
@modified    2026-09-08
@version     00.00.20.00020 [init]

CMAKE MODULE COMPLIANCE
[x] cmake_minimum_required(VERSION 3.20) as FIRST line
[x] project() with VERSION and LANGUAGES CXX
[x] message(STATUS) for configuration output
[x] add_library() with STATIC and EXPLICIT file list
[x] add_library(ase::markdown ALIAS ...) for namespace
[x] target_include_directories() with PUBLIC and generator expressions
[x] target_compile_features() with cxx_std_20
[x] target_link_libraries() with PUBLIC/PRIVATE separation
[x] include(GNUInstallDirs) for installation
[x] install(DIRECTORY include/ ...) for headers
]]

# Parse local VERSION (SSOT) if not building from root
if(NOT DEFINED ASE_MARKDOWN_VERSION)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
        message(FATAL_ERROR "VERSION file not found: ${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    endif()
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _VERSION_LINES)
    foreach(_LINE ${_VERSION_LINES})
        if(_LINE MATCHES "^ASE_MARKDOWN=(.*)$")
            set(ASE_MARKDOWN_VERSION "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    if(NOT DEFINED ASE_MARKDOWN_VERSION)
        message(FATAL_ERROR "ASE_MARKDOWN version not found in VERSION file")
    endif()
endif()

project(ase-markdown VERSION ${ASE_MARKDOWN_VERSION} LANGUAGES CXX)

# Generate version.hpp from template (SSOT)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/ase/markdown/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/ase/markdown/version.hpp"
    @ONLY
)

message(STATUS "Configuring ${PROJECT_NAME} v${PROJECT_VERSION}")

#[[
EXTERNAL DEPENDENCIES
cmark-gfm: CommonMark + GFM parser (C library, FetchContent cached in build/_deps/)
]]
# Layer 0 dependency: ase-alloc (for standalone build)
if(NOT TARGET ase::alloc)
    set(ASE_FOUNDATION_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../foundation)
    if(EXISTS "${ASE_FOUNDATION_PATH}/ase-alloc/CMakeLists.txt")
        add_subdirectory(${ASE_FOUNDATION_PATH}/ase-alloc ${CMAKE_BINARY_DIR}/_deps/ase-alloc-build)
    endif()
endif()

# Layer 0 dependency: ase-containers (for standalone build)
if(NOT TARGET ase::containers)
    set(ASE_FOUNDATION_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../foundation)
    if(EXISTS "${ASE_FOUNDATION_PATH}/ase-containers/CMakeLists.txt")
        add_subdirectory(${ASE_FOUNDATION_PATH}/ase-containers ${CMAKE_BINARY_DIR}/_deps/ase-containers-build)
    endif()
endif()

# Layer 0 dependency: ase-utils (for standalone build)
# Added 2026-08-20 for ase::utils::str_len in markdown_prs_fmtr.cpp - the bounded
# replacement the std-library rule prescribes for std::strlen. Same standalone shape as
# the two dependencies above, so a build of this module alone resolves it the same way.
if(NOT TARGET ase::utils)
    set(ASE_FOUNDATION_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../foundation)
    if(EXISTS "${ASE_FOUNDATION_PATH}/ase-utils/CMakeLists.txt")
        add_subdirectory(${ASE_FOUNDATION_PATH}/ase-utils ${CMAKE_BINARY_DIR}/_deps/ase-utils-build)
    endif()
endif()

if(NOT TARGET libcmark-gfm_static)
    include(FetchContent)
    FetchContent_Declare(cmark_gfm
        GIT_REPOSITORY https://github.com/github/cmark-gfm.git
        GIT_TAG 0.29.0.gfm.13
        GIT_SHALLOW TRUE
    )
    set(CMARK_TESTS OFF CACHE BOOL "" FORCE)
    set(CMARK_SHARED OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(cmark_gfm)

    # ── Real fix for upstream cmark-gfm/buffer.c -Wstringop-overflow ──
    # GCC's IPA/-O2 analysis flags cmark's intentional sentinel-buffer
    # pattern (cmark_strbuf__initbuf is a 1-byte placeholder, the write
    # at offset 1 only happens AFTER buf->ptr has been redirected to a
    # real allocation — the analyzer can't prove that across functions).
    # Suppression scoped to the single file via #pragma — narrowest scope
    # the GCC C dialect allows for an upstream-source false-positive.
    set(_CMARK_BUFFER "${cmark_gfm_SOURCE_DIR}/src/buffer.c")
    if(EXISTS "${_CMARK_BUFFER}")
        file(READ "${_CMARK_BUFFER}" _CMARK_BUFFER_SRC)
        if(NOT _CMARK_BUFFER_SRC MATCHES "Wstringop-overflow")
            file(WRITE "${_CMARK_BUFFER}"
                "#pragma GCC diagnostic ignored \"-Wstringop-overflow\"\n${_CMARK_BUFFER_SRC}")
            message(STATUS "Patched cmark-gfm buffer.c stringop-overflow false-positive")
        endif()
    endif()
endif()

#[[
LIBRARY DEFINITION
]]
add_library(${PROJECT_NAME} STATIC
    # Sources
    src/markdown_prs_base.cpp
    src/markdown_prs_fmtr.cpp
    src/markdown_prs_clot.cpp
    src/markdown_prs_math.cpp
    src/markdown_prs_nfnt.cpp
    src/markdown_prs_drct.cpp

    # Headers (for IDE integration)
    include/ase/markdown/markdown.hpp
    include/ase/markdown/frontmatter.hpp
    include/ase/markdown/types.hpp
    include/ase/markdown/ast.hpp
    include/ase/markdown/directives.hpp
)
add_library(ase::markdown ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

# Disable cmark-gfm's NODE_* short-name macros so they cannot collide with
# the ASE constexpr NODE_* identifiers in ase/markdown/types.hpp. Without this
# the macros redefine our identifiers as cmark enum values, which truncate to
# uint8_t producing wrong AST node types.
target_compile_definitions(${PROJECT_NAME} PRIVATE CMARK_NO_SHORT_NAMES)

# cmark-gfm uses old-style include_directories() (directory-scoped, not target-scoped).
# Headers are not propagated to downstream targets via target_link_libraries.
# Add include paths directly to ase-markdown so source files find cmark-gfm.h
target_include_directories(${PROJECT_NAME} PRIVATE
    ${cmark_gfm_SOURCE_DIR}/src
    ${cmark_gfm_BINARY_DIR}/src
    ${cmark_gfm_SOURCE_DIR}/extensions
    ${cmark_gfm_BINARY_DIR}/extensions
)

# Layer 0 + external dependencies
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ase::alloc
    PRIVATE
        ase::containers  # Layer 0 - Vector (g_directives buffer in markdown_prs_drct.cpp impl only)
        ase::utils       # Layer 0 - strops str_len (bounded key scan in markdown_prs_fmtr.cpp impl only)
        libcmark-gfm-extensions_static
        libcmark-gfm_static
)

#[[ VERBOTEN: No Layer 3+ dependencies ]]

# Tests
if(ASE_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    add_subdirectory(tests)
endif()

#[[
WASM BUILD TARGET (Emscripten)
Build with: emcmake cmake -B build-wasm && cmake --build build-wasm
Produces: ase-markdown.wasm + ase-markdown.js
]]
if(EMSCRIPTEN)
    add_executable(ase-markdown-wasm
        src/wasm/markdown_wasm.cpp
    )
    target_link_libraries(ase-markdown-wasm PRIVATE ${PROJECT_NAME})
    set_target_properties(ase-markdown-wasm PROPERTIES
        OUTPUT_NAME "ase-markdown"
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/wasm"
    )
    # embind since 2026-08-20. The entry point is a C++ function registered through
    # EMSCRIPTEN_BINDINGS, not a C symbol: it returns a std::string that embind marshals
    # into a JS string. That is why EXPORTED_FUNCTIONS and the ccall/cwrap/UTF8ToString
    # runtime methods are gone - the old list existed to hand JavaScript a raw pointer
    # plus _malloc/_free so it could release it, and there is no pointer any more.
    target_link_options(ase-markdown-wasm PRIVATE
        "SHELL:-lembind"
        "SHELL:-s MODULARIZE=1"
        "SHELL:-s EXPORT_NAME='AseMarkdown'"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s TOTAL_MEMORY=16MB"
    )
endif()

# Installation
include(GNUInstallDirs)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
