● All tests pass: 74/74 valid and 3/3 invalid. Stage 8.2 is complete.

Summary of changes to src/compiler.c:
- Added scope_start to CodeGen; initialized to 0 in codegen_init and reset per-function.
- codegen_find_var now walks backward so innermost shadow wins.
- count_declarations is now recursive, so nested-scope declarations get stack slots.
- Removed the allows_decl parameter from codegen_statement.
- Duplicate-declaration check in AST_DECLARATION now scans only [scope_start, local_count).
- AST_BLOCK now pushes/pops a scope via scope_start / local_count.

New tests: test_shadow_inner_scope__2.c, test_outer_var_in_inner_scope__1.c, test_invalid_3__duplicate_declaration.c.

✻ Churned for 1m 20s       