Summary

Stage 9.1 (refactor only) is complete. The 1,182-line src/compiler.c is now split into:

Headers (include/): token.h, ast.h, lexer.h, parser.h, codegen.h, ast_pretty_printer.h, util.h — each with include guards, exposing only shared types and the entry-point     
functions.

Sources (src/): compiler.c (slim, ~80-line main only), ast.c, lexer.c, parser.c, codegen.c, ast_pretty_printer.c, util.c. Module-private helpers (lexer_skip_whitespace, all  
parse_*/codegen_* helpers, operator_name, count_declarations, etc.) remain static.

Build: CMakeLists.txt updated to compile all 7 sources and add include/ to the include path; target name ccompiler preserved.

Verification: Clean build, zero warnings, all 83 tests pass — behavior unchanged.        