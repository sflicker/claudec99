# Milestone Summary

## stage-92: Self-Compile Validation — Complete (Full Self-Hosting)

stage-92 achieves full self-compilation of the ClaudeC99 compiler. C0 (built
with gcc) compiles its own source to produce C1, which passes all 1306 tests;
C1 then compiles itself to produce C2, which also passes all 1306 tests,
confirming the bootstrap reaches a stable fixed point.

Self-hosting came in two passes. A first validation pass surfaced and fixed
seven real defects/limits and isolated the principal blocker — struct/union
by-value parameters and returns (the lexer/parser `Token` interface, passed and
returned by value). That blocker was implemented in **stage 91-01**; a second
full bootstrap pass then surfaced six more silent codegen bugs, fixed here,
after which self-compilation succeeded.

- Tokenizer: No changes.
- Grammar/Parser: No language changes; the `for`-statement builder appends
  children via `ast_add_child` (required by the new dynamic AST children array).
- AST/Semantics: `ASTNode.children` converted from a fixed-size array (hard cap
  64) to a lazily-allocated, doubling dynamic array. Fixes silent truncation of
  large translation units, blocks, and switches (e.g., dropping `main` and other
  declarations past the 64th) — the bug that had masked the compiler's true
  (non-)self-hosting state in an earlier report.
- Codegen (pass 1): `codegen_emit_externs` suppresses duplicate/redundant
  `extern` directives for objects both declared and defined in the same
  translation unit (fixes NASM redefinition errors). Hoisted six block-scope
  `static const char *[]` register tables to file scope (block-scope static
  arrays not yet supported).
- Codegen (pass 2 — six silent bugs found once the bootstrap ran end-to-end):
  struct-by-value array-element assignment (`arr[i] = struct_func()`),
  member-dot assignment (`obj.member = struct_func()`), member-arrow assignment
  (`ptr->member = struct_func()`), and deref assignment (`*ptr = struct_func()`);
  `sizeof` of arrow-access array/struct/union members; and `sizeof` of
  subscripted-struct members.
- Codegen (linkage): added `is_static_linkage` field on `GlobalVar` and emitted
  `global` NASM directives for non-static file-scope variables so the bootstrap
  link resolves cross-module symbols. `sizeof` of a string literal now returns
  `strlen+1`.
- Constants: `MAX_STRING_LITERALS` 256 → 2048; `MAX_SWITCH_LABELS` 64 → 256;
  new `MAX_CALL_LAYOUT_ITEMS`.
- Stdlib: Added `strtol` and `strtoul` declarations to stub `stdlib.h`; updated
  `time.h` stubs.
- Version: Changed `main` signature to the `char **argv` form (the `T *name[]`
  element-typing path is not yet correct). `VERSION_STAGE` "00920000"; version
  bumped to 01.02 with the stage-92 marker.
- Tests: All 1306 tests pass (823 valid, 237 invalid, 82 integration, 43
  print_ast, 100 print_tokens, 21 print_asm) — under the gcc-built compiler and
  under the self-compiled C1/C2 binaries alike. No regressions. Updated two
  `print_asm` expected files for the new `global` directives.
- Docs: `docs/self-compilation-report.md` rewritten to record the achieved
  self-hosting state (two-pass story; C0→C1→C2 result table). README and grammar
  updated.
- Notes: **Self-compilation is now achieved and reproducible.** The current
  `src/` tree avoids two still-unsupported constructs — inline struct/union
  literals (`(T){ … }`) and block-scope `static` arrays — neither of which blocks
  the bootstrap.
