# Session Export — stage-182: ARM64 Integer Arithmetic and Local Variables (Phase 2)

**Date:** 2026-11-15
**Host:** macOS ARM64 (Darwin 25.5.0), Apple clang 21.0.0, NASM (Homebrew), Rosetta 2
**Spec:** `docs/stages/ClaudeC99-spec-stage-182-arm64-arithmetic-locals.md`

**Commits:**

| Commit | What |
|--------|------|
| `4556edd` | spec (planning, before this session) |
| `301938f` | code, tests, manifest, version, kickoff |
| `082da5b`, `5e1ce1b` | self-host checkpoints, committed by `build.sh` |
| `f65d7c5` | self-compilation report |
| `7bf2bae` | milestone, transcript, checklist, README |
| `6f3b57c` | roadmap: Phase 2 result, open items |

---

## 1. `/implement-stage ClaudeC99-spec-stage-182-arm64-arithmetic-locals`

`STAGE_DISPLAY: stage-182`

### Pre-implementation checks

- **Operator values:** `AST_BINARY_OP`, `AST_UNARY_OP` and the inc/dec nodes carry the token text in `value` (`"+"`, `"<<"`, `"++"`, …). The pretty printer's `ADD`/`SUBTRACT` names are display only.
- **Integer literals:** `decl_type` is the lexer type (`TYPE_INT`/`TYPE_LONG`/`TYPE_LONG_LONG`), and `is_unsigned` marks `U` literals. `value` holds the digits with the suffix stripped, so `strtoul(value, NULL, 0)` handles hex and octal.
- **Assignment has two AST shapes.**
  - Identifier form: `value` = name, with one child (the rhs).
  - General-lvalue form: `value` = NULL, with children (lhs, rhs).
  - Compound `op=` is lowered to `x = x op rhs` in both forms.
- **Other shapes:**
  - `AST_COMMA_EXPR` is binary and left-nested.
  - `int a, b;` produces `AST_DECL_LIST`.
- **Function return width:** comes from `func->decl_type`. The parser does not record `unsigned` on a function's return type.
- **Tier-2 set:** all 64 listed files exist. Their ASTs use only Block, Declaration, Assignment, Binary, Unary, Pre/PostfixIncDec, Comma, IntLiteral, VarRef and Return nodes, with no `DECL_LIST`, unsigned, char or short.

### Spec issues raised (none blocking)

1. "`test_chained_assignment_1__42` through `_5__4`" — the actual files are `_3__10` and `_4__9`. The real names are used.
2. Both assignment AST shapes are accepted when the target is a local `VAR_REF`.
3. `AST_DECL_LIST` is treated as a container of declarations ("multiple declarations per block").
4. `long long` is handled the same as `long`.
5. Non-int/long types, `static`/`extern` locals, unsigned and character literals, other return types, and non-local names are rejected with specific phrases.
6. Empty function bodies, which Stage 181 rejected, are now accepted.

### Kickoff

`docs/kickoffs/stage-182-kickoff.md` (`haiku-stage-artifact-writer`).

## 2. Implementation

| File | Change |
|------|--------|
| `include/codegen_arm64.h` | `Arm64Local {name, offset, is_long, scope_depth}`; `CodeGenArm64` gains `Vec locals`, `scope_depth`, `next_offset`, `frame_size`, `func_name`, `ret_is_long` |
| `src/codegen_arm64.c` | Phase 2 backend (below) |
| `src/version.c` | `01820000` |
| `test/valid/arm64_tiers.txt` | `# Tier 2 — Phase 2 (stage 182)`: 68 entries |
| New valid tests | `arithmetic/test_large_literals__17.c`, `arithmetic/test_long_to_int_truncate__42.c`, `arithmetic/test_main_falls_off_end__0.c`, `expressions/test_nested_block_shadow__42.c` |
| `test/integration/test_arm64_skeleton_asm/` | `local_var.c` deleted and replaced by `comparison.c` (`return 1 < 2;`) as the unsupported case; `big_literal.c` is now a positive check (`.s` contains `movz` and `movk`) |

The backend in `src/codegen_arm64.c`:

- **Width helper:** `arm64_expr_is_long` is pure. A literal's width comes from its `decl_type` and a local's from its declaration. Binary ops follow the usual conversions, a shift takes its left operand's width, and a comma takes its right operand's.
- **Constants:** `arm64_emit_load_imm` uses `mov` for 0..65535. Anything larger uses `movz` plus a `movk … lsl #16/#32/#48` for each further non-zero chunk.
- **Binary operators:** stack machine, as in the x86 backend:
  - `str x0, [sp, #-16]!` pushes the left operand
  - `mov x1, x0` then `ldr x0, [sp], #16` brings it back after the right operand
  - instructions: `add`/`sub`/`mul`/`sdiv`, `sdiv` + `msub` for `%`, `and`/`orr`/`eor`, `lsl`/`asr`
  - a shift count keeps its own width
- **Unary and conversions:** `neg`/`mvn` for `-`/`~`, and no code for unary `+`. `sxtw x0, w0` converts int to long; using `w0` truncates long to int.
- **Frame:**
  - A pre-scan gives every declaration its own slot (int 4 bytes, long 8 bytes, never reused).
  - The frame is rounded up to 16, and `sub sp` is emitted only when it is non-zero. Frames over 4095 bytes are an error.
  - Slots are addressed as `[x29, #-off]`.
- **Scopes:** a scope stack with innermost-first lookup. A declarator is in scope in its own initializer (C99 6.2.1p7).
- **Returns:** `return` converts to the function's width, then branches with `b L<func>_epilogue`. Falling off the end gives `mov w0, #0`, then the epilogue: `mov sp, x29`, `ldp x29, x30, [sp], #16`, `ret`.
- **Errors:** specific "not yet supported" phrases cover:
  - comparison, logical and logical-not operators
  - unsigned integer literals and global variables
  - static and extern locals
  - `<kind> type` and `<kind> return type`
  - function parameters and `return` without a value
- **Removed:** the Stage 181 literal-range error.

### Session notes

- **Deviation from the spec's text:** `ldur`/`stur` only reach -256. Slots deeper than 256 bytes are therefore addressed through `sub x9, x29, #off`, so frames up to the spec's 4095-byte limit actually assemble.
- **Unsigned function return types cannot be rejected,** because the parser does not record them. This is harmless here, because converting int to (unsigned) long is a sign extension either way.
- **First smoke test:** a mixed program (large literals, compound assignment, `++`/`--`, a shadowed block, comma, `<<=`) gave the expected exit code 17 on the first build.
- **Linux regression check:** my first try used `git worktree` plus `rm -rf` and was denied. I redid it with `git archive HEAD | tar -x` into the scratchpad, which built a Stage 181 compiler there.
- **Pushes:** `git push` was denied from the tool both times. You ran `! git push` yourself (`ec98298..7bf2bae`, then `7bf2bae..6f3b57c`).

## 3. Verification

| Check | Result |
|-------|--------|
| `TARGET=arm64-macos ARM64_TIER=2 ./test/valid/run_tests.sh` | `73 of 1293 tests in scope`, **73/73 pass** natively |
| `ARM64_TIER=1` | 5/5 |
| `test_arm64_skeleton_asm` | pass; `return 1 < 2;` → `arm64-macos: comparison operator not yet supported` |
| Default macOS suite (`x86_64-macos`, Rosetta) | **2170/2170**: 249 unit, 1293 valid, 261 invalid, 196 integration, 50 print-AST, 100 print-tokens, 21 print-asm |
| Linux regression | `--target=x86_64-linux` output for all 1946 test sources, compared against a Stage 181 build: 1706 produced `.asm`, **0 differences** |
| Self-host (`--mode=self-host`, macOS) | **C0, C1 and C2 each passed 2170/2170** (versions below) |
| C2-driven ARM64 run | the self-hosted C2 compiled the smoke test to ARM64 (exit 17), and ran tier 2 at 73/73 |

Self-host versions:

| Step | Version | Built by |
|------|---------|----------|
| C0 | `00.03.01820000.01323` | AppleClang 21.0.0 |
| C1 | `00.03.01820000.01324` | C0 |
| C2 | `00.03.01820000.01325` | C1 |

No source changes were needed during the bootstrap. The only linker output was the same harmless `ld` warnings as stages 180–181.

## 4. Documentation

- **`docs/self-compilation-report.md`:** stage 182 macOS self-host entry; no bootstrap issues (`f65d7c5`).
- **Written by `haiku-stage-artifact-writer`:**
  - `docs/milestones/stage-182-milestone.md`
  - `docs/sessions/stage-182-transcript.md`
  - `docs/outlines/checklist.md`: Stage 182 section; no TODO item completed
  - `README.md`: new "Through Stage 182" entry; the `--target` bullet now describes the Phase 2 scope
- **My correction:** the generated docs claimed the Linux output matched "except the `__TIME__` test and new tests". The comparison found 0 differences, so I fixed the wording in the README, checklist, milestone and transcript.
- **`docs/outlines/arm64-macos-backend-roadmap.md`** (`6f3b57c`):
  - Phase 2 marked ✅ with a Result bullet
  - the tier-2 row now says manifest-listed files
  - Phase 2 checked off in the progress table; "Next up" is now Phase 3 (stage 183)
  - open items updated
- **`docs/grammar.md`:** unchanged, since the grammar did not change.

## 5. Follow-ups

- Run the Linux x86-64 self-host (stages 180–182).
- Record `unsigned` on function return types in the parser, by Phase 7.
- Frames over 4095 bytes need a multi-instruction `sub sp` (large arrays, Phase 6).
- Compile-error line numbers are about 5 lines too high in both backends (from Stage 181).
- macOS `ld` warnings; `bin/cc99` macOS/ARM64 support.
- Stage 183 (Phase 3): comparisons and control flow.
