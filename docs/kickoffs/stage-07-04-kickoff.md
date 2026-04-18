❯ /implement-stage docs/stages/ClaudeC99-spec-stage-07-04-pretty-print-asm.md

Read 2 files (ctrl+o to expand)

● Now let me look at the existing tests to understand the test style.

Read 3 files, listed 2 directories (ctrl+o to expand)

● Now let me also check the CMakeLists.txt and build setup.

Read 1 file (ctrl+o to expand)

● Good. I now have a full picture of the project. Here's my summary:
                                                                                                                                                                          
---                                                                                                                                                                     
Stage 7.4: Pretty-Print AST — Summary

What the spec requires

1. New CLI option --print-ast — when provided, the compiler parses the input, prints a pretty-formatted AST to stdout, then exits successfully. No code generation      
   occurs.
2. AST pretty-printer — a recursive function that prints each node on its own line with 2-space indentation per nesting level, using descriptive labels (ProgramDecl,   
   FunctionDecl, Block, VariableDeclaration, IntLiteral, Binary: ADD, etc.)
3. Error handling — if parsing fails, report the error as usual; no AST is printed.
4. Tests — verify AST output for variable declarations, expressions, control flow (if/while/for), nested blocks, and confirm no .asm file is generated.

What changes from Stage 7.3

- Tokenizer: No changes.
- Parser: No changes.
- AST: No changes (we just read existing nodes).
- Code generation: No changes (just conditionally skipped).
- Main: Argument parsing changes from argc != 2 to support --print-ast as an optional flag.
- New function: ast_pretty_print() that walks the tree recursively.

Spec issues

I notice a few minor issues in the spec text:
- Line 15: "should exactly as in" — missing word "behave" (minor grammar issue, intent is clear)
- Line 84: "No not modify" — should be "Do not modify" (typo, intent is clear)

These are cosmetic — the intent is unambiguous, so I'll proceed.

Implementation plan

1. Update main() — parse --print-ast flag, adjust argument handling
2. Add ast_pretty_print() — recursive printer with operator name mapping
3. Add tests — new test scripts and test cases for --print-ast output
4. Verify — build and run all tests (existing + new)                                                                                                                    
                                                                                        