cmake_minimum_required(VERSION 3.16)
project(MacroLang LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)

find_package(BISON REQUIRED)
find_package(FLEX  REQUIRED)

BISON_TARGET(
    Parser
    src/jmal.y
    ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
)

FLEX_TARGET(
    Lexer
    src/jmal.l
    ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp
)

ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

add_executable(jmal
    src/main.cpp
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Lexer_OUTPUTS}
)

target_precompile_headers(jmal PRIVATE src/templates.hpp)

option(ASSERT_TODO "Downgrade TODO hits to warnings instead of aborting" OFF)

target_compile_options(jmal PRIVATE -Wall -Wextra)

if(ASSERT_TODO)
    target_compile_definitions(jmal PRIVATE ASSERT_TODO)
    target_compile_definitions(ast  PRIVATE ASSERT_TODO)
endif()

target_include_directories(jmal PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(jmal PRIVATE
    ${FLEX_LIBRARIES}
)