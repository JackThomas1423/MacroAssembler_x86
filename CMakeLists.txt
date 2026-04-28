cmake_minimum_required(VERSION 3.16)
project(MacroLang LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

BISON_TARGET(
    Parser
    jmal.y
    ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
)

FLEX_TARGET(
    Lexer
    jmal.l
    ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp
)

ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

add_executable(jmal
    main.cpp
    jmal_ast.hpp
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Lexer_OUTPUTS}
)

target_compile_options(jmal PRIVATE -fpermissive -lfl)

target_include_directories(jmal PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(jmal PRIVATE ${FLEX_LIBRARIES})