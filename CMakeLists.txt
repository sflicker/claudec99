cmake_minimum_required(VERSION 3.10)
project(ccompiler C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(ccompiler
    src/compiler.c
    src/ast.c
    src/ast_pretty_printer.c
    src/codegen.c
    src/lexer.c
    src/parser.c
    src/type.c
    src/util.c
)

target_include_directories(ccompiler PRIVATE include)
