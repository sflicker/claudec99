# Self-Compilation Diagnostic Report

**Date:** 2026-06-08
**Stage:** stage-96 (compile multiple source files per invocation)
**Compiler:** `build/ccompiler` (C0, gcc-built → C1 → C2 via bootstrap)
**Method:** `./build.sh --mode=self-host` (added in stage 94):
archives previous named binaries, saves GCC-built binary as `ccompiler-c0`,
bootstraps C0 → C1 (each source module compiled by `ccompiler` with 300 s
timeout guard, assembled with `nasm -f elf64`, linked with `gcc -no-pie`),
runs full test suite, makes a checkpoint commit, then bootstraps C1 → C2
and runs the full test suite again. Named copies are saved as
`build/ccompiler-c0/c1/c2`; `build/ccompiler` is left as C2.

## Status

**Fully self-hosting.** C0 (the gcc-built compiler) compiles its own source
to produce C1; C1 compiles the same source to produce C2. All three pass the
entire **1412-test** suite (1306 compiler tests + 106 new Vec unit tests) with
no regressions, confirming the compiler reproduces a working copy of itself
and that the bootstrap has reached a stable fixed point.

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
| 1 | `build.sh --mode=bootstrap` failed immediately: `error: include file not found: <stdio.h>` | Bootstrap script only passed `-I include` (project headers), not `-I test/include` (stub system headers). `bin/cc99` correctly appended `test/include` but `build.sh` did not mirror this. | Added `-I "$SCRIPT_DIR/test/include"` to `do_bootstrap_build` in `build.sh` |
| 2 | C1 and C2 showed `00000` as build number | `build.sh` never computed or passed `-DVERSION_BUILD`; cmake derives it from `git rev-list --count HEAD` at configure time but the bootstrap did not. | Added `git rev-list --count HEAD` computation and `-DVERSION_BUILD=${build_num}` to the compiler invocation in `do_bootstrap_build` (`build.sh`) |
| 3 | C1 and C2 reported identical version strings | No commit occurred between the C1 and C2 bootstrap runs, so both read the same git commit count. | Added a `git commit --allow-empty` checkpoint step after C1 passes in `--mode=self-host`; C2's build number is now always strictly greater than C1's |
| 4 | C0 and C1 reported identical version strings | No commit occurred between the cmake build and the first bootstrap run. | Added a `git commit --allow-empty` checkpoint step after C0 passes in `--mode=self-host`; C1's build number is now always strictly greater than C0's |
| 5 | BuiltBy token for C1/C2 omitted the build number (e.g. `ClaudeC99_v00_02_00940000` instead of `ClaudeC99_v00_02_00940000_00657`) | The regex in `do_bootstrap_build` matched only three dotted groups, discarding the fourth (build number). | Changed regex from `[0-9]+\.[0-9]+\.[0-9]+` to `[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+` to capture all four version groups. |

After fixes 1–5, all modules compiled, all tests passed, and C0/C1/C2 each
carry a distinct version string and a BuiltBy token that names the exact
compiler version (including build number) that produced them.

## Issues found during stage 95-08 self-hosting test

Two bootstrap failures were surfaced and fixed.

| # | Symptom | Root cause | Fix |
|---|---------|------------|-----|
| 1 | C0→C1 bootstrap compilation of `lexer.c` failed: `error: expected expression, got 'char'` | `lexer_free` contained `char *s = *(char **)vec_get(&lexer->str_pool, i);` — C0 cannot parse the combined dereference-of-cast pattern `*(T **)expr` in a single initializer (same class of bug as stage 95-05 `*(ASTNode **)` fix). | Split into two statements: `char **pp = (char **)vec_get(...); free(*pp);` (`src/lexer.c`) |
| 2 | C1 crashed: `internal error: strbuf_append_char: capacity overflow` for any string literal of 9 or more characters | `strbuf_append_char` checked `b->cap > (size_t)-1 / 2` to guard the doubling. C0 emits `cqo; idiv` (signed division) for `(size_t)-1 / 2` — the `is_unsigned` flag is not propagated through cast expressions — giving `(-1)/2 = 0`, so any non-zero `b->cap` (e.g. 8 after the first reallocation) triggered the false overflow. The identical pattern was fixed in `vec_push` during stage 95-05. | Rewrote all three affected checks (`strbuf_append_char`, `strbuf_null_terminate`, `strbuf_append_n`) to copy `b->cap` and `(size_t)-1` to local `size_t` variables before the division — matching the vec.c pattern that correctly generates unsigned `div`. (`src/strbuf.c`) |

After both fixes all 1478 tests passed at C0, C1, and C2.

## Issues found during stage 96 self-hosting test

None. The `parser_free`/`codegen_free`/`reset_error_state` lifecycle additions
and the `owned_strings` Vec in `CodeGen` produced no bootstrap failures.
The `codegen_free` loop over `owned_strings` uses the two-statement
`char **pp = ...; char *s = *pp;` split pattern (avoiding the
`*(T **)vec_get(...)` form C0 cannot parse) as prescribed by the spec's
bootstrap caveat, and passed C0 compilation without incident.
All 1483 tests passed at C0, C1, and C2.

## Issues found during stage 97 self-hosting test

None. The designated-initializer implementation (parser, codegen for local/global structs and arrays) compiled cleanly under C0. All new codegen code uses fixed-size arrays (`MAX_STRUCT_FIELDS_DESIGNATED = 64`, `MAX_ARRAY_ELEMS_DESIGNATED = 1024`) rather than VLAs to maintain self-hosting compatibility. All 1501 tests passed at C0, C1, and C2.

## Issues found during stage 106 self-hosting test

None. The `restrict` keyword changes (TOKEN_RESTRICT in lexer/parser) and the
stub header additions are pure C-source-visible changes — the compiler's own
source code does not use `restrict`, and the new header declarations add no
code to the compiler binary. The `long long` return-value bug fix (adding
TYPE_LONG_LONG/TYPE_UNSIGNED_LONG_LONG to rhs_is_long checks in six codegen
sites) produces no behavioural change on the compiler's own source because the
compiler's internal functions do not return `long long`. All 1607 tests passed
at C0, C1, and C2 with no source changes needed during the bootstrap.

## Result (stage 106)

**Date:** 2026-06-12

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.01060000.00848` | `GNU_13_3_0` | 1607/1607 |
| C1 | `build/ccompiler-c1` | `00.02.01060000.00849` | `ClaudeC99_v00_02_01060000_00848` | 1607/1607 |
| C2 | `build/ccompiler-c2` | `00.02.01060000.00850` | `ClaudeC99_v00_02_01060000_00849` | 1607/1607 |

## Issues found during stage 107 self-hosting test

None. The `inline` keyword changes (TOKEN_INLINE in lexer/parser),
`test/include/assert.h`, the `va_copy` codegen fix, and the `__FILE__`/`__LINE__`
expansion fix in `expand_macros_text` are all C-source-visible changes that the
compiler's own source code does not exercise. The new static globals
(`g_expand_source_path`, `g_expand_current_line`) in `preprocessor.c` are
straightforward additions with no interaction with the existing bootstrap path.
All 1615 tests passed at C0, C1, and C2 with no source changes needed during
the bootstrap.

## Result (stage 107)

**Date:** 2026-06-13

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.01070000.00855` | `GNU_13_3_0` | 1615/1615 |
| C1 | `build/ccompiler-c1` | `00.02.01070000.00856` | `ClaudeC99_v00_02_01070000_00855` | 1615/1615 |
| C2 | `build/ccompiler-c2` | `00.02.01070000.00857` | `ClaudeC99_v00_02_01070000_00856` | 1615/1615 |

## Issues found during stage 108 self-hosting test

None. The `#elifdef`/`#elifndef` additions are confined to `src/preprocessor.c`
and `src/version.c`. The compiler's own source code does not use either directive;
the new branches are reached only if the preprocessor encounters those keywords,
so there is no code path change during bootstrap. All 1621 tests passed at C0,
C1, and C2 with no source changes needed during the bootstrap.

## Result (stage 108)

**Date:** 2026-06-13

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.01080000.00860` | `GNU_13_3_0` | 1621/1621 |
| C1 | `build/ccompiler-c1` | `00.02.01080000.00861` | `ClaudeC99_v00_02_01080000_00860` | 1621/1621 |
| C2 | `build/ccompiler-c2` | `00.02.01080000.00862` | `ClaudeC99_v00_02_01080000_00861` | 1621/1621 |

## Issues found during stage 112 self-hosting test

Two bootstrap failures were surfaced and fixed.

| # | Symptom | Root cause | Fix |
|---|---------|------------|-----|
| 1 | C1 bootstrap of `src/codegen.c` failed: `error: expected ']', got '+'` | New local array declarations used `[MAX_CALL_LAYOUT_ITEMS + 2]` as the dimension. ClaudeC99 does not support arithmetic expressions as local array dimension sizes (only single-identifier macros or literals). | Changed all three `[MAX_CALL_LAYOUT_ITEMS + 2]` declarations to the literal `[26]` (= 24 + 2). |
| 2 | `test_fp_varargs__0` segfault: `movaps` general-protection fault | The variadic register-save area was allocated as 184 bytes; the xmm0 slot was computed at `[rbp - 184 + 48] = [rbp - 136]`. With `rbp mod 16 = 0` (true for all functions: `call` + `push rbp` = 16 bytes, restoring alignment), `[rbp - 136] mod 16 = 8`, which is not 16-byte aligned as `movaps` requires. | Changed the save-area allocation from 184 to 176 bytes. With rso = 176, xmm0 slot at `[rbp - 128]` has `128 mod 16 = 0`. The total frame size is still rounded to 16 bytes by the existing `(stack_size + 15) & ~15` rounding. |
| 3 | `printf("%.0f\n", d)` printed `0` instead of `42` | `compute_call_layout` used `call_node->children[i]->decl_type` to classify variadic extra args. For `AST_VAR_REF` expression nodes, `decl_type` is 0 (TYPE_VOID), so the double was classified as a GP arg instead of XMM. | `compute_call_layout` now accepts a `const TypeKind *actual_types` array; the call site computes `actual_types[i] = expr_result_type(cg, node->children[i])` before calling `compute_call_layout`, providing correct types for all args including variadic extras. |

After all three fixes, 1650/1650 tests passed at C0, C1, and C2 with no further changes needed.

## Result (stage 112)

**Date:** 2026-06-13

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.01120000.00889` | `GNU_13_3_0` | 1650/1650 |
| C1 | `build/ccompiler-c1` | `00.02.01120000.00890` | `ClaudeC99_v00_02_01120000_00889` | 1650/1650 |
| C2 | `build/ccompiler-c2` | `00.02.01120000.00891` | `ClaudeC99_v00_02_01120000_00890` | 1650/1650 |

## Issues found during stage 114 self-hosting test

None. The new codegen paths (FP array subscript read/write, nested brace local
array initialization, mixed FP/int ternary normalization, string literal
subscript, global nested-brace array initialization) all involve data types and
patterns that the compiler's own source code does not exercise: no `float` or
`double` arrays, no nested-brace multidimensional initializers, no FP branches
in ternary expressions, and no string-literal subscripting. The `emit_local_array_init`
and `emit_global_array_elements` helper additions are pure additions with no
interaction with existing bootstrap paths. The parser change (allowing
`AST_STRING_LITERAL` as subscript base) is only triggered by string literals
used as subscript targets, which the compiler source never does.
All 1841 tests passed at C0, C1, and C2 with no source changes needed during
the bootstrap.

## Result (stage 114)

**Date:** 2026-06-13

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.01140000.00899` | `GNU_13_3_0` | 1841/1841 |
| C1 | `build/ccompiler-c1` | `00.02.01140000.00900` | `ClaudeC99_v00_02_01140000_00899` | 1841/1841 |
| C2 | `build/ccompiler-c2` | `00.02.01140000.00901` | `ClaudeC99_v00_02_01140000_00900` | 1841/1841 |

## Issues found during stage 111 self-hosting test

None. The FP comparison and boolean-context additions (`emit_fp_bool_to_rax`,
`emit_cond_cmp_zero` FP branch, FP comparison block in `AST_BINARY_OP`, FP
logical NOT in unary `!`, FP condition in `AST_CONDITIONAL_EXPR`) all involve
code paths that the compiler's own source code does not exercise: the compiler
source uses no `float` or `double` comparisons, no FP boolean conditions, and
no FP logical NOT. The new SSE2 comparison instructions (`ucomiss`/`ucomisd`)
are only emitted when a comparison operator is applied to a float/double operand,
which never occurs during a self-host build.
All 1643 tests passed at C0, C1, and C2 with no source changes needed.

## Result (stage 111)

**Date:** 2026-06-13

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.01110000.00881` | `GNU_13_3_0` | 1643/1643 |
| C1 | `build/ccompiler-c1` | `00.02.01110000.00882` | `ClaudeC99_v00_02_01110000_00881` | 1643/1643 |
| C2 | `build/ccompiler-c2` | `00.02.01110000.00883` | `ClaudeC99_v00_02_01110000_00882` | 1643/1643 |

## Issues found during stage 110 self-hosting test

None. The FP arithmetic additions (SSE2 binary arithmetic, UAC, unary minus,
casts, implicit int→FP coercion) all involve code paths that the compiler's
own source code does not exercise: the compiler source uses no `float` or
`double` arithmetic, no FP unary minus, and no FP casts. The two new CodeGen
fields (`fp_sign_mask_f32_emitted`, `fp_sign_mask_f64_emitted`) are initialized
to 0 and never set during a self-host build; the updated `codegen_emit_fp_literals`
emits the sign-mask entries only when these flags are non-zero. The extended
`emit_fp_widen_if_needed` and `emit_convert` FP branches are only entered when
an FP type is encountered, which the compiler source never produces.
All 1635 tests passed at C0, C1, and C2 with no source changes needed.

## Result (stage 110)

**Date:** 2026-06-13

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.01100000.00872` | `GNU_13_3_0` | 1635/1635 |
| C1 | `build/ccompiler-c1` | `00.02.01100000.00873` | `ClaudeC99_v00_02_01100000_00872` | 1635/1635 |
| C2 | `build/ccompiler-c2` | `00.02.01100000.00874` | `ClaudeC99_v00_02_01100000_00873` | 1635/1635 |

## Issues found during stage 109 self-hosting test

None. The `float`/`double` type additions (new token kinds, AST node, type
singletons, lexer FP literal scanning, parser type-specifier and primary
expression paths, codegen FP literal pool, and load/store helpers) all involve
code paths that the compiler's own source does not exercise: the compiler source
uses no `float` or `double` types or literals. The `FpLiteral` Vec added to
`CodeGen`, the `fp_literals` field in `codegen_init`/`codegen_free`, and the new
`codegen_emit_fp_literals` emission function are all inert during a self-host
build. All 1627 tests passed at C0, C1, and C2 with no source changes needed
during the bootstrap.

## Result (stage 109)

**Date:** 2026-06-13

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.01090000.00867` | `GNU_13_3_0` | 1627/1627 |
| C1 | `build/ccompiler-c1` | `00.02.01090000.00868` | `ClaudeC99_v00_02_01090000_00867` | 1627/1627 |
| C2 | `build/ccompiler-c2` | `00.02.01090000.00869` | `ClaudeC99_v00_02_01090000_00868` | 1627/1627 |

## Issues found during stage 104 self-hosting test

None. The new constant-expression evaluator functions (`eval_const_relational`,
`eval_const_equality`, `eval_const_logical_and`, `eval_const_logical_or`,
`eval_const_conditional`) are purely token-stream recursive arithmetic with no dynamic
allocation and no new AST node types. The `eval_const_init` additions (`<`, `<=`, `>`,
`>=`, `==`, `!=`, `&&`, `||`, `AST_CONDITIONAL_EXPR`) are pure integer comparisons on
values already evaluated. The compiler's own constant expressions (case labels, enum
values, block-scope statics) use none of the new operators, so no compiler source changes
were needed. The additive/shift precedence fix does not change evaluated values in the
compiler's own source (no existing use mixes `+`/`-` with `<<`/`>>` without parentheses).
All 1584 tests passed at C0, C1, and C2.

## Result (stage 104)

**Date:** 2026-06-11

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.01040000.00837` | `GNU_13_3_0` | 1584/1584 |
| C1 | `build/ccompiler-c1` | `00.02.01040000.00838` | `ClaudeC99_v00_02_01040000_00837` | 1584/1584 |
| C2 | `build/ccompiler-c2` | `00.02.01040000.00839` | `ClaudeC99_v00_02_01040000_00838` | 1584/1584 |

## Issues found during stage 103 self-hosting test

None. The new `eval_const_init` helper is purely recursive arithmetic with no dynamic
allocation and no new AST node types. The compiler's own source uses block-scope `static`
scalars exclusively with simple literals (0, 1, -1, NULL), all of which go through the
`AST_INT_LITERAL` branch — identical behavior to the old code. All 1569 tests passed at
C0, C1, and C2.

## Result (stage 103)

**Date:** 2026-06-11

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.01030000.00831` | `GNU_13_3_0` | 1569/1569 |
| C1 | `build/ccompiler-c1` | `00.02.01030000.00832` | `ClaudeC99_v00_02_01030000_00831` | 1569/1569 |
| C2 | `build/ccompiler-c2` | `00.02.01030000.00833` | `ClaudeC99_v00_02_01030000_00832` | 1569/1569 |

## Issues found during stage 102 self-hosting test

None. The new designated-init handling, struct/union slot emission, and
2D row emission in `codegen_emit_local_statics` all use patterns already
verified in the global array path. The multidimensional `.bss` fixes use
only `sv->full_type->size` (an existing field) and `resb` (already used
for struct/union bss). The compiler's own source uses no block-scope
static arrays of structs, unions, or multiple dimensions, so no compiler
source changes were needed. All 1560 tests passed at C0, C1, and C2.

## Result (stage 102)

**Date:** 2026-06-11

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.01020000.00824` | `GNU_13_3_0` | 1560/1560 |
| C1 | `build/ccompiler-c1` | `00.02.01020000.00825` | `ClaudeC99_v00_02_01020000_00824` | 1560/1560 |
| C2 | `build/ccompiler-c2` | `00.02.01020000.00826` | `ClaudeC99_v00_02_01020000_00825` | 1560/1560 |

## Issues found during stage 101 self-hosting test

None. The new array/struct/union static-local registration paths use
`vec_push` (the established pattern), and `codegen_emit_local_statics`
uses fixed-size `slots[]` arrays bounded by `MAX_ARRAY_ELEMS_DESIGNATED`
(already present for the global array path). The compiler's own source
uses block-scope `static` scalars and pointers extensively but no
`static` array or struct locals, so the bootstrap cycle required no
source changes. All 1552 tests passed at C0, C1, and C2.

## Result (stage 101)

**Date:** 2026-06-10

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.01010000.00819` | `GNU_13_3_0` | 1552/1552 |
| C1 | `build/ccompiler-c1` | `00.02.01010000.00820` | `ClaudeC99_v00_02_01010000_00819` | 1552/1552 |
| C2 | `build/ccompiler-c2` | `00.02.01010000.00821` | `ClaudeC99_v00_02_01010000_00820` | 1552/1552 |

## Issues found during stage 100 self-hosting test

None. The `eval_const_primary` sizeof extension and the `parse_external_declaration` path replacements are purely parser-level changes with no new AST node types, no dynamic allocation, and no codegen changes. All new code uses the existing `parse_type_name` / `type_size` / `lexer_store_bytes` helpers already verified self-hosting. The `sizeof(void *)` fix in `parse_primary` uses `parse_type_name` followed by a `t->kind == TYPE_VOID` check — the same pattern used for array incomplete-type detection. All 1544 tests passed at C0, C1, and C2.

## Result (stage 100)

**Date:** 2026-06-10

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.01000000.00812` | `GNU_13_3_0` | 1544/1544 |
| C1 | `build/ccompiler-c1` | `00.02.01000000.00813` | `ClaudeC99_v00_02_01000000_00812` | 1544/1544 |
| C2 | `build/ccompiler-c2` | `00.02.01000000.00814` | `ClaudeC99_v00_02_01000000_00813` | 1544/1544 |

## Issues found during stage 99 self-hosting test

None. The extended `eval_const_expr` evaluator is purely recursive arithmetic with no dynamic allocation and no new AST nodes, so it compiled cleanly under C0. The forward-declaration enum tag path uses `vec_push` exactly as existing struct/union tag paths do. All 1531 tests passed at C0, C1, and C2.

## Result (stage 99)

**Date:** 2026-06-10

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00990000.00805` | `GNU_13_3_0` | 1531/1531 |
| C1 | `build/ccompiler-c1` | `00.02.00990000.00806` | `ClaudeC99_v00_02_00990000_00805` | 1531/1531 |
| C2 | `build/ccompiler-c2` | `00.02.00990000.00807` | `ClaudeC99_v00_02_00990000_00806` | 1531/1531 |

## Issues found during stage 98 self-hosting test

None. The compound literal implementation compiled cleanly under C0. The compound literal stack-offset pre-scan and codegen use the standard `for`-over-children pattern (no fixed-size temp arrays, no VLAs). The `parse_postfix` compound literal detection uses an `int saved_pos`/`Token saved_token` checkpoint to restore the lexer when `(type-name)` is not followed by `{`. All 1521 tests passed at C0, C1, and C2.

## Result (stage 98)

**Date:** 2026-06-10

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00980000.00799` | `GNU_13_3_0` | 1521/1521 |
| C1 | `build/ccompiler-c1` | `00.02.00980000.00800` | `ClaudeC99_v00_02_00980000_00799` | 1521/1521 |
| C2 | `build/ccompiler-c2` | `00.02.00980000.00801` | `ClaudeC99_v00_02_00980000_00800` | 1521/1521 |

## Result (stage 97)

**Date:** 2026-06-10

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00970000.00792` | `GNU_13_3_0` | 1501/1501 |
| C1 | `build/ccompiler-c1` | `00.02.00970000.00793` | `ClaudeC99_v00_02_00970000_00792` | 1501/1501 |
| C2 | `build/ccompiler-c2` | `00.02.00970000.00794` | `ClaudeC99_v00_02_00970000_00793` | 1501/1501 |

## Result (stage 96)

**Date:** 2026-06-08

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00960000.00786` | `GNU_13_3_0` | 1483/1483 |
| C1 | `build/ccompiler-c1` | `00.02.00960000.00787` | `ClaudeC99_v00_02_00960000_00786` | 1483/1483 |
| C2 | `build/ccompiler-c2` | `00.02.00960000.00788` | `ClaudeC99_v00_02_00960000_00787` | 1483/1483 |

## Issues found during stage 95-12 self-hosting test

None. The preprocessor `eval_cond_unary` StrBuf conversion and the `SwitchCtx`
dynamic label-table conversion produced no bootstrap failures. The new
`(SwitchLabel *)vec_get(...)` accesses use the cast-of-pointer form (then `->`),
not the `*(T **)expr` cast-of-deref pattern C0 cannot parse, so no split was
needed. A dangling-pointer bug in the switch lifecycle (the post-body
`vec_free(&ctx->entries)` reused a `ctx` pointer invalidated when nested
switches reallocated `switch_stack`) was caught by the existing
`test_switch_nested_20_deep` test during the GCC test run and fixed before the
bootstrap by re-fetching the live top element via `vec_get`. All 1481 tests
passed at C0, C1, and C2.

## Result (stage 95-12)

**Date:** 2026-06-08

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00951200.00778` | `GNU_13_3_0` | 1481/1481 |
| C1 | `build/ccompiler-c1` | `00.02.00951200.00779` | `ClaudeC99_v00_02_00951200_00778` | 1481/1481 |
| C2 | `build/ccompiler-c2` | `00.02.00951200.00780` | `ClaudeC99_v00_02_00951200_00779` | 1481/1481 |

## Issues found during stage 95-11 self-hosting test

None. The codegen struct `const char *` pointer migrations and `user_labels` Vec
conversion produced no bootstrap failures. All 1479 tests passed at C0, C1, and C2.

## Result (stage 95-11)

**Date:** 2026-06-08

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00951100.00767` | `GNU_13_3_0` | 1479/1479 |
| C1 | `build/ccompiler-c1` | `00.02.00951100.00768` | `ClaudeC99_v00_02_00951100_00767` | 1479/1479 |
| C2 | `build/ccompiler-c2` | `00.02.00951100.00769` | `ClaudeC99_v00_02_00951100_00768` | 1479/1479 |

## Issues found during stage 95-10 self-hosting test

None. The parser.h `const char *` pointer migration produced no bootstrap failures.
All 1479 tests passed at C0, C1, and C2.

## Result (stage 95-10)

**Date:** 2026-06-07

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00951000.00756` | `GNU_13_3_0` | 1479/1479 |
| C1 | `build/ccompiler-c1` | `00.02.00951000.00757` | `ClaudeC99_v00_02_00951000_00756` | 1479/1479 |
| C2 | `build/ccompiler-c2` | `00.02.00951000.00758` | `ClaudeC99_v00_02_00951000_00757` | 1479/1479 |

## Issues found during stage 95-09 self-hosting test

None. The `ASTNode.value` pointer migration produced no bootstrap failures.
All 1479 tests passed at C0, C1, and C2.

## Result (stage 95-09)

**Date:** 2026-06-07

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00950900.00741` | `gcc_Ubuntu_13_3_0` | 1479/1479 |
| C1 | `build/ccompiler-c1` | `00.02.00950900.00742` | `ClaudeC99_v00_02_00950900_00741` | 1479/1479 |
| C2 | `build/ccompiler-c2` | `00.02.00950900.00743` | `ClaudeC99_v00_02_00950900_00742` | 1479/1479 |

C0, C1, and C2 each compile successfully with distinct version strings and
full build provenance. The compiler is self-hosting and the bootstrap is
reproducible.

## Result (stage 95-08)

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00950800.00731` | `gcc_Ubuntu_13_3_0` | 1478/1478 |
| C1 | `build/ccompiler-c1` | `00.02.00950800.00732` | `ClaudeC99_v00_02_00950800_00731` | 1478/1478 |
| C2 | `build/ccompiler-c2` | `00.02.00950800.00733` | `ClaudeC99_v00_02_00950800_00732` | 1478/1478 |

C0, C1, and C2 each compile successfully with distinct version strings and
full build provenance. The compiler is self-hosting and the bootstrap is
reproducible.

## Issues found during stage 95-07 self-hosting test

None. The `MAX_SWITCH_DEPTH` Vec conversion and `MAX_CALL_LAYOUT_ITEMS` bounds check
produced no bootstrap failures. The pattern used — local `SwitchCtx` variable + `vec_push`
+ single-star `vec_get` cast — is well within what C0 can parse. All 1471 tests passed
at each stage.

## Result (stage 95-07)

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00950700.00718` | `GNU_13_3_0` | 1471/1471 |
| C1 | `build/ccompiler-c1` | `00.02.00950700.00719` | `ClaudeC99_v00_02_00950700_00718` | 1471/1471 |
| C2 | `build/ccompiler-c2` | `00.02.00950700.00720` | `ClaudeC99_v00_02_00950700_00719` | 1471/1471 |

C0, C1, and C2 each compile successfully with distinct version strings and
full build provenance. The compiler is self-hosting and the bootstrap is
reproducible.

## Issues found during stage 95-06 self-hosting test

None. All four Vec conversions (PARSER_MAX_STRUCT_FIELDS, MAX_BREAK_DEPTH,
PARSER_MAX_TYPEDEFS, PARSER_MAX_FUNCTIONS) produced no bootstrap failures.
The C0 bootstrap patterns used (local BreakFrame/FuncSig/TypedefEntry/StructField
variables with vec_push, and single-star vec_get casts) are well within what C0
can parse. All 1471 tests passed at each stage.

## Result (stage 95-06)

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00950600.00711` | `GNU_13_3_0` | 1471/1471 |
| C1 | `build/ccompiler-c1` | `00.02.00950600.00712` | `ClaudeC99_v00_02_00950600_00711` | 1471/1471 |
| C2 | `build/ccompiler-c2` | `00.02.00950600.00713` | `ClaudeC99_v00_02_00950600_00712` | 1471/1471 |

C0, C1, and C2 each compile successfully with distinct version strings and
full build provenance. The compiler is self-hosting and the bootstrap is
reproducible.

## Issues found during stage 95-05 self-hosting test

Two latent bugs were surfaced and fixed during this bootstrap run.

| # | Symptom | Root cause | Fix |
|---|---------|------------|-----|
| 1 | C0→C1 bootstrap compilation of `src/codegen.c` failed: `error: expected expression, got ')'` at a bogus line number | The pattern `*(ASTNode **)vec_get(...)` causes a parse error in C0. When `parse_unary` consumes the outer `*`, the inner `parse_primary` sees `(` and consumes it, calling `parse_expression` with `ASTNode` as the current token. Since `ASTNode` is a typedef-name (not starting `(`), `parse_cast` cannot recognize the cast and `parse_expression` treats `ASTNode * *` as a multiplication followed by a dereference-of-`)`, yielding "expected expression, got ')'". (The bogus line number is a known lexer-position drift in the save/restore paths of `parse_cast` and `parse_assignment_expression`.) | Split `*(ASTNode **)vec_get(...)` into two statements: `ASTNode **s_ptr = (ASTNode **)vec_get(...); ASTNode *s = *s_ptr;` (`src/codegen.c`) |
| 2 | C1 crashed with `vec_push: capacity overflow` when compiling functions with ≥ 9 parameters | `vec_push` checked `v->cap > (size_t)-1 / 2` to detect capacity near overflow before doubling. Our compiler emits `cqo; idiv` (signed division) for this expression because the `is_unsigned` flag is not propagated through cast expressions (`AST_CAST` codegen sets `result_type` but not `is_unsigned`). Signed division of `-1 / 2 = 0`, so any non-zero `cap` value triggered the false overflow. The identical bug was fixed in `vec_reserve` during stage 95-04. | Copy `v->cap` and `(size_t)-1` to local `size_t` variables before the comparison and doubling — local `size_t` declarations correctly generate unsigned division. (`src/vec.c`) |

After both fixes all 1471 tests passed at C0, C1, and C2.

## Result (stage 95-05)

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00950500.00698` | `GNU_13_3_0` | 1471/1471 |
| C1 | `build/ccompiler-c1` | `00.02.00950500.00699` | `ClaudeC99_v00_02_00950500_00698` | 1471/1471 |
| C2 | `build/ccompiler-c2` | `00.02.00950500.00700` | `ClaudeC99_v00_02_00950500_00699` | 1471/1471 |

C0, C1, and C2 each compile successfully with distinct version strings and
full build provenance. The compiler is self-hosting and the bootstrap is
reproducible.

## Issues found during stage 95-04 self-hosting test

Two issues were discovered and fixed during this bootstrap run. Both are
pre-existing latent defects that stage 95-04 was the first to exercise.

| # | Symptom | Root cause | Fix |
|---|---------|------------|-----|
| 1 | Link error `undefined reference to 'vec_init'`, `vec_push`, `vec_get` for C1 | `src/vec.c` and `src/strbuf.c` were missing from `SRC_FILES` in `build.sh` since stage 95-02 when they were added; the normal cmake build picked them up via `CMakeLists.txt`, but the bootstrap script's hand-maintained list was not updated. | Added `src/strbuf.c` and `src/vec.c` to `SRC_FILES` in `build.sh`. |
| 2 | C1 crash: `internal error: vec_reserve: capacity overflow` on union/enum/local-static tests | `vec_reserve` contains `(size_t)-1 / v->elem_size`; our compiler emits `cqo; idiv` (signed division) for this expression because `v->elem_size` is a struct member accessed through a pointer and the unsigned type is not correctly propagated to the division codegen. The signed quotient of `(unsigned long)-1 / 264` ≈ `0`, so `min_cap (8) > 0` triggered the fatal error. | Rewrote the overflow check in `vec_reserve` to copy struct members to explicit local `size_t` variables first — local `size_t` variables correctly generate `div`. |

After both fixes all 1471 tests passed at C0, C1, and C2.

## Result (stage 95-04)

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00950400.00683` | `GNU_13_3_0` | 1471/1471 |
| C1 | `build/ccompiler-c1` | `00.02.00950400.00684` | `ClaudeC99_v00_02_00950400_00683` | 1471/1471 |
| C2 | `build/ccompiler-c2` | `00.02.00950400.00685` | `ClaudeC99_v00_02_00950400_00684` | 1471/1471 |

C0, C1, and C2 each compile successfully with distinct version strings and
full build provenance. The compiler is self-hosting and the bootstrap is
reproducible.

## Issues found during stage 95-03 self-hosting test

None. `strbuf.c` is not compiled by the self-hosted compiler (it is only a
utility library consumed by future growable-storage stages). The unit tests are
compiled by the system GCC and do not participate in the C1/C2 bootstrap.
All 1471 tests passed at each stage.

## Result (stage 95-03)

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00950300.00672` | `GNU_13_3_0` | 1471/1471 |
| C1 | `build/ccompiler-c1` | `00.02.00950300.00673` | `ClaudeC99_v00_02_00950300_00672` | 1471/1471 |
| C2 | `build/ccompiler-c2` | `00.02.00950300.00674` | `ClaudeC99_v00_02_00950300_00673` | 1471/1471 |

C0, C1, and C2 each compile successfully with distinct version strings and
full build provenance. The compiler is self-hosting and the bootstrap is
reproducible.

## Issues found during stage 95-02 self-hosting test

None. `vec.c` is not compiled by the self-hosted compiler (it is only a utility
library consumed by future growable-storage stages). The new `test/unit/`
suite is compiled by the system GCC in the unit test runner and does not
participate in the C1/C2 bootstrap. All 1412 tests passed at each stage.

## Result (stage 95-02)

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00950200.00665` | `GNU_13_3_0` | 1412/1412 |
| C1 | `build/ccompiler-c1` | `00.02.00950200.00666` | `ClaudeC99_v00_02_00950200_00665` | 1412/1412 |
| C2 | `build/ccompiler-c2` | `00.02.00950200.00667` | `ClaudeC99_v00_02_00950200_00666` | 1412/1412 |

C0, C1, and C2 each compile successfully with distinct version strings and
full build provenance. The compiler is self-hosting and the bootstrap is
reproducible. Timeout guards (300 s per file) added in stage 93 were confirmed
active — all modules compiled well within the limit.

## Result (stage 94)

| Step | Binary | Version | BuiltBy | Tests |
|------|--------|---------|---------|-------|
| C0 | `build/ccompiler-c0` | `00.02.00940000.00657` | `GNU_13_3_0` | 1306/1306 |
| C1 | `build/ccompiler-c1` | `00.02.00940000.00658` | `ClaudeC99_v00_02_00940000_00657` | 1306/1306 |
| C2 | `build/ccompiler-c2` | `00.02.00940000.00659` | `ClaudeC99_v00_02_00940000_00658` | 1306/1306 |

C0, C1, and C2 each compile successfully with distinct version strings and
full build provenance: C1's BuiltBy names C0's exact version, and C2's BuiltBy
names C1's exact version. The compiler is self-hosting and the bootstrap is
reproducible. Timeout guards (300 s per file) added in stage 93 were confirmed
active — all modules compiled well within the limit.

## Known limitation surfaced by self-compilation

Self-hosting works against the current `src/` tree as written, which avoids
two still-unsupported constructs: **inline struct/union literals** (`(T){ … }`)
and **block-scope `static` arrays** (the six register tables in `codegen.c`
were hoisted to file scope rather than relying on this). Both remain future
work but do not block the bootstrap.
