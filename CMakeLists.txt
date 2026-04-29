cmake_minimum_required(VERSION 3.16)
project(MacroLang LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

add_library(ast src/jmal_ast.cpp)

target_include_directories(ast PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

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

option(JMAL_NO_TODO_ASSERT "Downgrade TODO hits to warnings instead of aborting" OFF)

target_compile_options(jmal PRIVATE -Wall -Wextra)

if(JMAL_NO_TODO_ASSERT)
    target_compile_definitions(jmal PRIVATE JMAL_NO_TODO_ASSERT)
    target_compile_definitions(ast  PRIVATE JMAL_NO_TODO_ASSERT)
endif()

target_include_directories(jmal PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(jmal PRIVATE
    ast
    ${FLEX_LIBRARIES}
)