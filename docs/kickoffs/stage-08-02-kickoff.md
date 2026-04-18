❯ /implement-stage docs/stages/ClaudeC99-spec-stage-08-02-nested-scopes.md

Read 9 files, listed 4 directories (ctrl+o to expand)

● I'll start by reading the spec and exploring how scoping is currently handled.

Searched for 1 pattern, listed 1 directory (ctrl+o to expand)

● I have full context. Here's the stage kickoff.
                                                                                                                                                                                                                         
---                                                                                                                                                                                                                    
1. Spec summary

Stage 8.2 adds nested block scopes and variable shadowing:

- Each { ... } introduces a new scope.
- A variable declared in a scope is visible in that scope and all nested (child) scopes.
- A variable in an inner scope may shadow an outer one with the same name.
- Name resolution walks from innermost to outermost; the nearest declaration wins.
- Redeclaring a variable in the same scope is still an error.
- No tokenizer or grammar changes.

2. What must change from stage 8.1

All changes are in the code generator — the semantic analysis layer:

┌────────────────────────────────────────────────────────────────────────────────────────────────────┬───────────────────────────────────────────────────────────────────────────────────────┐                         
│                                          Current behavior                                          │                                   Required behavior                                   │
├────────────────────────────────────────────────────────────────────────────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────────┤                         
│ codegen_statement(..., allows_decl) rejects declarations in nested scopes.                         │ Allow declarations in any scope. Remove the allows_decl flag.                         │
├────────────────────────────────────────────────────────────────────────────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────────┤                         
│ codegen_find_var does a linear forward search over a flat locals[].                                │ Search backward so the most-recent (innermost) declaration wins under shadowing.      │                         
├────────────────────────────────────────────────────────────────────────────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────────┤                         
│ Duplicate-decl check uses codegen_find_var, which sees all scopes and would block legal shadowing. │ Only check names added in the current scope (indices ≥ scope_start).                  │                         
├────────────────────────────────────────────────────────────────────────────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────────┤                         
│ count_declarations only counts top-level declarations in a block.                                  │ Count recursively, so stack space is allocated for declarations in nested blocks too. │
├────────────────────────────────────────────────────────────────────────────────────────────────────┼───────────────────────────────────────────────────────────────────────────────────────┤                         
│ AST_BLOCK handler just iterates children.                                                          │ Push a new scope on entry (save local_count, bump scope_start), pop on exit.          │
└────────────────────────────────────────────────────────────────────────────────────────────────────┴───────────────────────────────────────────────────────────────────────────────────────┘

No AST changes. No parser changes. No tokenizer changes.

3. Test plan

- Valid, new:
    - test_shadow_inner_scope__2.c — inner int a = 2; shadows outer int a = 1;, returns 2 (spec example 1).
    - test_outer_var_in_inner_scope__1.c — inner scope reads outer a and returns it (spec example 2, adapted to return the value).
- Invalid, new:
    - test_invalid_3__duplicate_declaration.c variant: duplicate declaration in the same nested block — still errors with "duplicate declaration".
- Regressions: existing tests (top-level duplicate decl, nested blocks without shadowing, if/while/for) must still pass.

  ---                                                                                                                                                                                                                    
Shall I proceed with the codegen implementation (changes 1–5 above)?

✻ Brewed for 2m 2s                      
                           