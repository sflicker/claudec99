cmake_minimum_required(VERSION 3.10)
project(ccompiler C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Derive a build number from the git commit count so --version reflects
# the current revision automatically on each cmake configure step.
execute_process(
    COMMAND git rev-list --count HEAD
    OUTPUT_VARIABLE VERSION_BUILD
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
if(NOT VERSION_BUILD)
    set(VERSION_BUILD 0)
endif()

add_executable(ccompiler
    src/compiler.c
    src/ast.c
    src/ast_pretty_printer.c
    src/codegen.c
    src/lexer.c
    src/parser.c
    src/preprocessor.c
    src/type.c
    src/util.c
    src/version.c
)

target_compile_definitions(ccompiler PRIVATE VERSION_BUILD=${VERSION_BUILD})
target_include_directories(ccompiler PRIVATE include)
