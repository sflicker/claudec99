/implement-stage ClaudeC99-spec-stage-09-01-reorganize-code-into-multiple-files.md

Read 6 files, listed 3 directories (ctrl+o to expand)

● I have a complete picture. Let me first present the summary and plan before implementing anything.

Summary of Stage 9.1 Spec

Goal: Pure refactor — split the monolithic src/compiler.c (1182 lines) into multiple .c modules under src/ and shared headers under include/. No behavioral changes           
whatsoever.

Required file layout

include/                                                                                                                                                                      
├── token.h   
├── ast.h
├── lexer.h
├── parser.h
├── codegen.h                                                                                                                                                               
├── ast_pretty_printer.h
└── util.h                                                                                                                                                                  
src/                                                                                                                                                                          
├── compiler.c        (thin entry: just main())
├── ast.c             (ast_new, ast_add_child, ast_free)                                                                                                                    
├── lexer.c           (Lexer + tokenization)                                                                                                                                
├── parser.c          (recursive descent parser)                                                                                                                            
├── codegen.c         (CodeGen + x86_64 NASM emission)                                                                                                                      
├── ast_pretty_printer.c  (operator_name, ast_pretty_print)
└── util.c            (read_file, make_output_path)

Key rules from the spec

1. Headers contain only what must be shared across modules (shared structs, enums, typedefs, function declarations).
2. Module-private helpers stay static inside their .c.
3. compiler.c becomes a thin entry point.
4. CMakeLists.txt must add all sources and add include/ to the include path; keep target name ccompiler.
5. CLI, parsing, AST output, codegen, pretty-print, and tests must be unchanged.

Mapping current compiler.c sections → new files

- Section 1 (Token defs) → include/token.h
- Section 2 (AST node defs + helpers) → include/ast.h + src/ast.c
- Section 3 (Lexer) → include/lexer.h + src/lexer.c
- Section 4 (Parser) → include/parser.h + src/parser.c
- Section 5 (AST Pretty Printer) → include/ast_pretty_printer.h + src/ast_pretty_printer.c
- Section 6 (Code Generator) → include/codegen.h + src/codegen.c
- Section 7 (File I/O Utilities) → include/util.h + src/util.c
- Section 8 (Main) → src/compiler.c

Header-vs-static decisions

- Public (in headers): all typedefs used across modules (Token, TokenType, ASTNode, ASTNodeType, Lexer, Parser, CodeGen, LocalVar), constants used across modules             
  (AST_MAX_CHILDREN, MAX_LOCALS), and the function entry points another module calls (lexer_init, lexer_next_token, parser_init, parse_program, ast_new, ast_add_child,
  ast_free, ast_pretty_print, codegen_init, codegen_program, read_file, make_output_path).
- Private (static inside .c): lexer_skip_whitespace, all parse_* helpers except parse_program and parser_init/parser_expect, operator_name, ast_print_indent, all codegen_*
  helpers except codegen_init/codegen_program, count_declarations.

What changes from the previous stage

Nothing functional. Only:
- File partitioning per above.
- New header files with shared declarations and include guards.
- CMakeLists.txt updated: add all new sources, target_include_directories(ccompiler PRIVATE include).
- Each .c will #include only what it needs (own header + dependencies).

Test plan

Re-run existing test suites under test/valid/, test/invalid/, test/print_ast/ after build. No source-level test additions are warranted (spec says behavior must not change).
                                                                                                                                                                                
---                                                                                                                                                                           
Pausing here. Want me to proceed with creating the headers and source files? I'll do it in two batches:
1. All seven headers + src/ast.c, src/util.c, src/lexer.c (small modules) + updated CMakeLists.txt
2. src/parser.c, src/codegen.c, src/ast_pretty_printer.c, slim src/compiler.c, then build & run the tests.

✻ Worked for 44s                               