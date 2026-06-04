# Self-Compilation Diagnostic Report

**Date:** 2026-06-04
**Stage:** stage-92 (self-compile validation)
**Compiler:** `build/ccompiler` (C0, gcc-built) bootstrapping itself
**Method:** full bootstrap via `bin/cc99 -Iinclude -o build/ccompiler-c1 src/*.c`
(`ccompiler` → `nasm -f elf64` → `gcc -no-pie` link), **not** per-module
`.asm` production alone.

## Why the previous report was wrong

The 2026-06-03 report claimed all 10 modules "compiled cleanly" and that the
compiler was fully self-hosting. That conclusion was an artifact of the
measurement: it only checked that `ccompiler` emitted a `.asm` file and exited
0 for each module. It never **assembled**, **linked**, or **ran** the result.

Crucially, `ast_add_child` silently dropped any child beyond
`AST_MAX_CHILDREN` (64). Every translation unit accumulates its top-level
declarations as children of a single `AST_TRANSLATION_UNIT` node, so for a
real file like `compiler.c` (≫64 top-level decls) **everything past the 64th
declaration — including `main` — was discarded with no diagnostic.** The
resulting `.asm` was a small, truncated stub that happened to assemble, so the
old per-module check reported a false PASS.

A true bootstrap (compile → assemble → link → test) exposes the real picture.

## Blockers found and fixed in stage-92

All fixes below keep the host (gcc-built) test suite green at **1302/1302**.

| # | Symptom | Root cause | Fix |
|---|---------|------------|-----|
| 1 | `util.asm`: label `g_error_src_line` inconsistently redefined (NASM) | A global both *defined* in a TU and declared `extern` via its own header emitted both a label definition and an `extern` directive | `codegen_emit_externs`: suppress `extern` for any object also defined in the TU; dedupe repeated externs (`src/codegen.c`) |
| 2 | `main` and most top-level decls silently missing → link error `undefined reference to 'main'` | `ast_add_child` silently dropped children past the fixed `AST_MAX_CHILDREN` (64); translation units, large blocks, and big switches overflowed | `children` is now a lazily-allocated, doubling dynamic array (`include/ast.h`, `src/ast.c`); `for`-node builder appends via `ast_add_child` (`src/parser.c`) |
| 3 | `codegen.c`: too many string literals (max 256) | `MAX_STRING_LITERALS` sized for toy programs; `codegen.c` has ~750 literal occurrences (pool does not dedupe) | Raised default to 2048 (`include/constants.h`) |
| 4 | `compiler.c`: too many case/default labels in switch (max 64) | `MAX_SWITCH_LABELS` too small; `token_type_name()` switches over ~83 token kinds | Raised default to 256 (`include/constants.h`) |
| 5 | `codegen.c`: static local arrays not yet supported | Six block-scope `static const char *…[6]` register tables | Hoisted to file-scope `static` arrays (semantics unchanged) (`src/codegen.c`) |
| 6 | `codegen.c` / `compiler.c`: call to undefined function `strtol` | Stub `stdlib.h` did not declare `strtol`/`strtoul` | Added both declarations (`test/include/stdlib.h`) |
| 7 | `compiler.c`: `strcmp` arg "expected pointer, got integer" | The `T *name[]` parameter form (`char *argv[]`) mis-typed its element on subscript; only the `T **name` spelling works | Changed `main`'s signature to the equivalent `char **argv` (`src/compiler.c`) |

After fixes 1–7, modules `ast.c`, `ast_pretty_printer.c`, `codegen.c`,
`compiler.c`, `type.c`, `util.c`, and `version.c` compile, assemble, and link
cleanly during the bootstrap.

## Remaining blocker — struct by value (deferred)

The bootstrap currently halts in `lexer.c`:

```
lexer.c:116: error: '.' applied to non-struct/union 'token'
```

The lexer/parser interface passes and returns the `Token` struct **by value**,
which the compiler does not yet support (a documented "Not yet supported"
feature). Affected functions:

- `Token lexer_next_token(Lexer *lexer)` — returns `Token` by value (the core
  lexer → parser interface; `Token` is a large, memory-class struct).
- `static Token finalize(Token token)` — takes **and** returns `Token` by
  value (called from ~80 sites in `lexer.c`).
- `static Token parser_expect(Parser *parser, TokenType type)` — returns
  `Token` by value.

Resolving this requires either (a) implementing SysV AMD64 struct-by-value
parameters and return values (memory-class structs returned via a hidden
pointer in `rdi`), or (b) refactoring the lexer/parser interface to pass
`Token` through pointers/out-parameters. Both are larger than a validation
pass and are deferred to a dedicated future stage.

## Caveat

The bootstrap stops at the first error per module (`bin/cc99` runs
`ccompiler` with its default `--max-errors`). Validation therefore reached
`lexer.c` and has **not** yet exercised `lexer.c` (post-`finalize`),
`parser.c`, or `preprocessor.c` end-to-end. Additional blockers may exist
beyond the struct-by-value gap and will surface only once it is resolved.

## Status

**Not yet self-hosting.** Stage-92 validation surfaced and fixed seven real
defects/limits (including a silent AST-truncation bug that invalidated the
prior report) and isolated the principal remaining blocker (struct by value).
Full self-compilation remains pending.
