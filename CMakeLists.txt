cmake_minimum_required(VERSION 3.10)
project(ccompiler C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

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
    src/strbuf.c
    src/vec.c
    src/version.c
    src/optimize.c
    src/peephole.c
)

# Record which compiler built the binary.  Dots in the version number are
# replaced with underscores so the value is a valid C token for stringification.
string(REPLACE "." "_" COMPILER_VER_CLEAN "${CMAKE_C_COMPILER_VERSION}")
set(BUILTBY_TOKEN "${CMAKE_C_COMPILER_ID}_${COMPILER_VER_CLEAN}")

target_compile_definitions(ccompiler PRIVATE
    VERSION_BUILD=${VERSION_BUILD}
    VERSION_BUILTBY=${BUILTBY_TOKEN}
)
target_include_directories(ccompiler PRIVATE include)
target_compile_options(ccompiler PRIVATE -Wall -Wextra -pedantic)

add_custom_command(TARGET ccompiler POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ccompiler> ${CMAKE_SOURCE_DIR}/bin/ccompiler
)
