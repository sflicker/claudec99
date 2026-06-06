# Self-Compilation Diagnostic Report

**Date:** 2026-06-06
**Stage:** stage-94 (self-host validation and implement-stage skill test)
**Compiler:** `build/ccompiler` (C0, gcc-built → C1 → C2 via bootstrap)
**Method:** `./build.sh --mode=bootstrap` (stage 93/94 bootstrap driver):
each source module compiled by `ccompiler` with timeout guard (300 s),
assembled with `nasm -f elf64`, all objects linked with `gcc -no-pie`.
Repeated twice (C0→C1, then C1→C2) to confirm fixed-point stability.

**Stage 93 method note:** The original `build.sh --mode=bootstrap` command
introduced in stage 93 was missing `-I test/include`; the stub system headers
(`stdio.h`, `stdlib.h`, etc.) live there and are required when compiling the
compiler's own source. Discovered and fixed in stage 94 (see "Issues found"
below); all stage-94 bootstrap runs used the corrected script.

## Status

**Fully self-hosting.** C0 (the gcc-built compiler) compiles its own source
to produce C1; C1 compiles the same source to produce C2. Both C1 and C2
pass the entire **1306-test** suite with no regressions, confirming the
compiler reproduces a working copy of itself and that the bootstrap has
reached a stable fixed point.

Getting here took two passes. The first validation pass (this report's
earlier revision) surfaced and fixed seven real defects/limits — including a
silent AST-truncation bug that had invalidated an even earlier "self-hosting"
claim — and isolated the principal remaining blocker: struct/union by-value
parameters and returns, used pervasively by the lexer/parser `Token`
interface. That blocker was implemented in **stage 91-01**, after which a
second full bootstrap pass surfaced six more silent codegen bugs (all
struct-by-value or `sizeof` edge cases), which were fixed in **stage 92**.

## Why the very first report was wrong

The original report claimed all 10 modules "compiled cleanly" and that the
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

## Pass 1 — blockers found and fixed during validation

These seven fixes kept the host (gcc-built) test suite green and got the
bootstrap from "truncated stub" to "halts only on struct-by-value".

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
`compiler.c`, `type.c`, `util.c`, and `version.c` compiled, assembled, and
linked cleanly; the bootstrap then halted in `lexer.c`.

## The principal blocker — struct by value (resolved in stage 91-01)

Pass 1 halted in `lexer.c`:

```
lexer.c:116: error: '.' applied to non-struct/union 'token'
```

The lexer/parser interface passes and returns the `Token` struct **by value**,
which the compiler did not yet support. Affected functions:

- `Token lexer_next_token(Lexer *lexer)` — returns `Token` by value (the core
  lexer → parser interface; `Token` is a large, memory-class struct).
- `static Token finalize(Token token)` — takes **and** returns `Token` by
  value (called from ~80 sites in `lexer.c`).
- `static Token parser_expect(Parser *parser, TokenType type)` — returns
  `Token` by value.

**Stage 91-01** resolved this by implementing the System V AMD64 struct/union
by-value calling convention: register-class aggregates (≤16 bytes) travel in
registers, memory-class aggregates (>16 bytes — `Token` among them) travel
through a hidden pointer. The work added `emit_struct_addr`,
`emit_struct_copy` (`rep movsb`), `compute_struct_ret_bytes`, and
`claim_struct_ret_temp`, plus a shared call-layout helper used by both call
sites and prologues. With this in place the bootstrap proceeded past
`lexer.c` and exercised `parser.c` and `preprocessor.c` end-to-end.

## Pass 2 — silent codegen bugs found once the bootstrap ran end-to-end

With struct-by-value working, a second full bootstrap pass (and running the
resulting C1) surfaced six more bugs that had been latent because no host test
exercised them. All six were fixed in **stage 92**:

| # | Bug | Fix area |
|---|-----|----------|
| 1 | Struct-by-value assignment via subscript (`arr[i] = struct_func()`) | `src/codegen.c` |
| 2 | Struct-by-value assignment via member-dot (`obj.m = struct_func()`) | `src/codegen.c` |
| 3 | Struct-by-value assignment via member-arrow (`ptr->m = struct_func()`) | `src/codegen.c` |
| 4 | Struct-by-value assignment via deref (`*ptr = struct_func()`) | `src/codegen.c` |
| 5 | `sizeof` of arrow-access array/struct/union members | `src/codegen.c` |
| 6 | `sizeof` of subscripted-struct members | `src/codegen.c` |

Stage 92 also added `MAX_CALL_LAYOUT_ITEMS` (`include/constants.h`), an
`is_static_linkage` field on `GlobalVar`, and `global` NASM directives for
non-static file-scope variables (so the bootstrap link resolves cross-module
symbols), and fixed `sizeof` of a string literal to return `strlen+1`.

## Issues found during stage 94 self-hosting test

| # | Symptom | Root cause | Fix |
|---|---------|------------|-----|
| 1 | `build.sh --mode=bootstrap` failed immediately: `error: include file not found: <stdio.h>` | Bootstrap script only passed `-I include` (project headers), not `-I test/include` (stub system headers). `bin/cc99` correctly appended `test/include` but `build.sh` did not mirror this. | Added `-I "$SCRIPT_DIR/test/include"` to the `ccompiler` invocation in `do_bootstrap_build` (`build.sh`) |

After fix #1, all modules compiled and all tests passed on both C1 and C2.

## Result

| Step | Compiler | Built by | Tests |
|------|----------|----------|-------|
| C0   | `build/ccompiler` | GCC 13.3.0 (stage-94 normal build) | 1306/1306 |
| C1   | self-compiled from C0 via `build.sh --mode=bootstrap` | C0 (ClaudeC99_v00_02_00940000) | 1306/1306 |
| C2   | self-compiled from C1 via `build.sh --mode=bootstrap` | C1 (ClaudeC99_v00_02_00940000) | 1306/1306 |

C0, C1, and C2 each compile successfully with identical test results. The
compiler is self-hosting and the bootstrap is reproducible. Timeout guards
(300 s per file) added in stage 93 were exercised in this run and confirmed
active — all modules compiled well within the limit.

## Known limitation surfaced by self-compilation

Self-hosting works against the current `src/` tree as written, which avoids
two still-unsupported constructs: **inline struct/union literals** (`(T){ … }`)
and **block-scope `static` arrays** (the six register tables in `codegen.c`
were hoisted to file scope rather than relying on this). Both remain future
work but do not block the bootstrap.
