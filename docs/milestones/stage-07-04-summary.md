Summary of changes

src/compiler.c:
1. Stage header updated to 7.4
2. New Section 5: AST Pretty Printer — added operator_name() (maps + → ADD, < → LESSTHAN, etc.), ast_print_indent(), and ast_pretty_print() which recursively prints the
   AST tree with 2-space indentation
3. Updated main() — argument parsing now supports --print-ast flag before or after the source file. When set, it prints the AST and exits with code 0, skipping code    
   generation entirely.

test/print_ast/ — new test directory with:
- 6 test cases covering: variable declarations, expressions (with precedence), if/else, while loops, for loops, nested blocks
- Each test has a .c input and .expected output file
- run_tests.sh runner that also verifies no .asm file is produced

✻ Crunched for 1m 52s      