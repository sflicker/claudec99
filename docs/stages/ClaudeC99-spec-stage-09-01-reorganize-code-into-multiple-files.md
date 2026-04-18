# Claude C99 Stage 9.1

Task: Reorganize project code into multiple files.

## Description
Currently the source code of the compiler is in a single file `compiler.c`. In this stage
break up this file into multiple code files along functionality areas including header files.
The CMake build files should also be updated. A separate directory `include` for the
header files needs to be created.

New header files will contain structs, enums and function declarations that 
need to be shared with other modules.

A potential list of header files in the include directory:
token.h
ast.h
lexer.h
parser.h
codegen.h
ast_pretty_printer.h
util.h

Source files should contain private data structures along with the functions
for that module.
A potential list of Source files:
compiler.c - should contain the main() function for the application.
ast.c
lexer.c
parser.c
ast_pretty_printer.c
codegen.c
util.c - should contain the file I/O utilities

The CMakeLists.txt file will also need to have the appropriate updates.

