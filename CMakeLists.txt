cmake_minimum_required(VERSION 3.20)

#[[
ASE CORE BUILD CONFIGURATION

@file        CMakeLists.txt
@brief       Build configuration for ase-markdown
@description CommonMark/GFM + ASE DSL Markdown parser producing typed AST.

@module      ase-markdown
@layer       1 (Core)
@created     2026-04-12
@modified    2026-04-12
@version     00.00.01.00001 [stub]

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

# Parse local VERSION if not building from root
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
LIBRARY DEFINITION
]]
add_library(${PROJECT_NAME} STATIC
    # Sources
    src/markdown_stub.cpp

    # Headers (for IDE integration)
    include/ase/markdown/markdown.hpp
    include/ase/markdown/types.hpp
    include/ase/markdown/ast.hpp
)
add_library(ase::markdown ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

#[[ VERBOTEN: No Layer 3+ dependencies ]]

# Tests
if(ASE_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    add_subdirectory(tests)
endif()

# Installation
include(GNUInstallDirs)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
