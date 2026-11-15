# Stage 182 Kickoff: ARM64 Integer Arithmetic and Local Variables (Phase 2)

## Derived Values

| Item | Value |
|------|-------|
| STAGE_LABEL | stage-182 |
| STAGE_DISPLAY | stage-182 |
| Spec | `docs/stages/ClaudeC99-spec-stage-182-arm64-arithmetic-locals.md` |
| Backend changed | `include/codegen_arm64.h` (`CodeGenArm64` gains `Vec locals`, scope depth, current function name, return width, `frame_size`), `src/codegen_arm64.c` |
| Manifest | `test/valid/arm64_tiers.txt`: new `# Tier 2 — Phase 2 (stage 182)` section, 68 `2 <path>` lines |
| New tests | `test/valid/arithmetic/test_large_literals__17.c`, `test/valid/arithmetic/test_long_to_int_truncate__42.c`, `test/valid/expressions/test_nested_block_shadow__42.c`, `test/valid/arithmetic/test_main_falls_off_end__0.c` |
| Updated integration test | `test/integration/test_arm64_skeleton_asm/` (`big_literal.c` becomes a positive check; unsupported-construct case becomes `return 1 < 2;`) |
| VERSION_STAGE | `01820000` (stage 181 set `01810000`) |
| Expected output | Native `TARGET=arm64-macos ARM64_TIER=2 ./test/valid/run_tests.sh`: 73 of N in scope, 73/73 passing; `ARM64_TIER=1` still 5/5; Linux `.asm` byte-identical to stage 181 (except `__TIME__` test and new tests); default macOS suite 2170 (2166 + 4 new valid); macOS self-host C0→C1→C2 passes; Linux self-host recorded as pending |

## 1. Spec Summary

Stage 182 is Phase 2 of the ARM64/macOS roadmap. It extends the `arm64-macos` backend from a function body that is a single `return <literal>;` to straight-line signed integer code. Stage 181 remains the baseline for everything not listed here.

Supported in this stage:

- **Types:** signed `int` (32-bit), `long` and `long long` (64-bit, treated as the same width).
- **Declarations:** block-scope `int`/`long`/`long long` locals, with or without an initializer; multiple declarations per block; nested `{ }` blocks with shadowing.
- **Statements:** `return <expr>;`, expression statements, declarations, nested blocks; any number of statements per body.
- **Primary:** integer literals of any size (`movz`/`movk`), local variable references, parentheses.
- **Binary:** `+ - * / %`, `& | ^`, `<< >>` (signed; `>>` is `asr`).
- **Unary:** `-`, `~`, unary `+`.
- **Assignment:** `=` to a local. Compound `op=` arrives already lowered by the parser. Assignment yields the assigned value, so chaining works.
- **Inc/dec:** prefix and postfix `++`/`--` on locals.
- **Comma:** left then right; the value is the right operand.
- **Functions:** no parameters, returning `int` or `long`. Several per translation unit are allowed, but only `main` runs (calls are Phase 4).

Still rejected with `error: arm64-macos: <construct> not yet supported`: comparisons and logical operators, `?:`, `if`/loops/`switch`/`goto` (Phase 3); parameters and calls (Phase 4); globals and string literals (Phase 5); pointers, arrays, structs, unions, `&`, `*`, `sizeof` (Phase 6); `char`, `short`, `unsigned`, unsigned literals, casts (Phase 7); floating point (Phase 8); varargs (Phase 9).

The Stage 181 error `integer literal <v> out of range for this stage` is removed.

The x86 targets (`x86_64-linux`, `x86_64-macos`) must not change. Auto-detection is unchanged.

### Codegen design highlights

- **Value model:** every expression leaves its result in `w0` (int) or `x0` (long). `int` results have an undefined upper half of `x0` until sign-extended. Expression type comes from a pure helper `static int arm64_expr_is_long(CodeGenArm64 *cg, ASTNode *e)`.
- **Conversions:** int→long is `sxtw x0, w0`. long→int truncates by using `w0`.
- **Binary ops:** stack-machine style. Evaluate left, convert, `str x0, [sp, #-16]!`; evaluate right, convert; `mov x1, x0`; `ldr x0, [sp], #16`; apply the op. `%` is `sdiv r2, r0, r1` then `msub r0, r2, r1, r0`.
- **Constants:** `mov` for `0..65535`; otherwise `movz` plus `movk` for each further non-zero 16-bit chunk.
- **Frame:** pre-scan gives every declaration its own slot (`int` 4 bytes/4-aligned, `long` 8 bytes/8-aligned, no slot reuse). Frame rounded to 16. Frame above 4095 bytes is an error: `arm64-macos: stack frame larger than 4095 bytes not yet supported`. Addressing is `[x29, #-off]`.
- **Scopes:** a scope stack for name lookup, innermost first.
- **Returns and epilogue:** `return` converts to the function's width and branches to `b L<func>_epilogue`. Fall-through emits `mov w0, #0`, then `L<func>_epilogue:` with `mov sp, x29; ldp x29, x30, [sp], #16; ret`.
- **Memory:** `Vec` from `include/vec.h`; everything freed in `codegen_arm64_free`.

## 2. Required Tokenizer Changes

None.

## 3. Required Parser Changes

None. `docs/grammar.md` stays as is.

## 4. Required AST Changes

None. The backend consumes the existing AST. Assignment has two shapes that the backend must both accept:

- Named form: `node->value` holds the name, one child is the right-hand side.
- General lvalue form: `value` is NULL, children are lhs and rhs.

Both are accepted when the target is a local `VAR_REF`. Any other lvalue is rejected.

## 5. Required Code Generation Changes

- `include/codegen_arm64.h`: add the new `CodeGenArm64` fields listed above.
- `src/codegen_arm64.c`:
  - `arm64_expr_is_long()` pure type helper.
  - `emit_load_imm` (mov / movz / movk, two's-complement pattern at destination width).
  - Binary, unary, assignment, `++`/`--`, comma, and return codegen per the spec.
  - Frame pre-scan, prologue/epilogue, `[x29, #-off]` addressing, scope stack.
  - Remove the Stage 181 out-of-range literal error.
  - Rejection messages with specific phrases where useful: `unsigned type`, `char type`, `function parameter`, `comparison operator`, `global variable`, and so on.
- Expected output for `int main(void) { return 42; }` is unchanged from stage 181.
- No changes to `src/codegen.c`, the x86 peephole pass, or any tokenizer, parser, or AST file.

## 6. Test Plan

### Manifest

- `test/valid/arm64_tiers.txt`: add the 64 existing tier-2 files plus the 4 new tests, 68 entries, under `# Tier 2 — Phase 2 (stage 182)`. `test_octal_zero__0` is already tier 1 and gets no second entry.
- Tier-2 set: `arithmetic/` (41 files, including `test_octal_zero__0`), `assignment/` (16), `expressions/` (8). The spec lists these file by file. Remaining tests in these directories stay out of the manifest until their phase lands.
- No runner changes. `ARM64_TIER` still defaults to `1`.

### New tests (target-neutral, join tier 2)

- (a) `test/valid/arithmetic/test_large_literals__17.c`: `long x = 4886718345L; int y = 305419896; return (x - y) % 256;` (expected 17). Exercises `movz`/`movk` at both widths.
- (b) `test/valid/arithmetic/test_long_to_int_truncate__42.c`: `long big = 4294967338L; int t = big; return t;` (expected 42).
- (c) `test/valid/expressions/test_nested_block_shadow__42.c`: outer `x = 40`, inner `int x = 1; x = x + 1;`, `return x + 2;` (expected 42).
- (d) `test/valid/arithmetic/test_main_falls_off_end__0.c`: `int a = 5; a = a * 2;` with no return (expected 0).

### Integration test

- `test/integration/test_arm64_skeleton_asm/`: `big_literal.c` compiles successfully under `--target=arm64-macos`, and its `.s` contains `movz` and `movk`. The unsupported-construct case changes from a local variable to `return 1 < 2;` and expects `arm64-macos:` and `not yet supported`.

### Verification

1. Native ARM64 (Apple Silicon): `TARGET=arm64-macos ARM64_TIER=2 ./test/valid/run_tests.sh` reports 73 of N in scope and 73/73 passing. `ARM64_TIER=1` still gives 5/5.
2. macOS default target: full portable suite passes at 2170.
3. Linux regression: generated `.asm` for every test source under `x86_64-linux` is byte-identical to stage 181, except the `__TIME__` test and the new tests.
4. macOS self-host: `./build.sh --mode=self-host` passes C0→C1→C2, including the expanded `src/codegen_arm64.c`.
5. Linux self-host: remains pending. Record the status in the milestone.

## Findings and Decisions

Decisions made before implementation:

1. **Chained assignment filenames.** The spec gives `test_chained_assignment_1__42` through `test_chained_assignment_5__4`. The real files are `_1__42`, `_2__42`, `_3__10`, `_4__9`, `_5__4`. The manifest uses the real names.
2. **Assignment AST shapes.** Both shapes described in section 4 are accepted for local `VAR_REF` targets. Other lvalues are rejected.
3. **Multiple declarations.** `int a, b;` produces `AST_DECL_LIST`. The backend treats it as a container of `AST_DECLARATION`s, and each gets its own slot. No tier-2 test uses it.
4. **`long long` is `long`.** Both are 64-bit. Return types other than signed `int`/`long`/`long long`, and declarations with unsigned, char, short, pointer, array, struct, float types, or static/extern storage, are rejected with specific "not yet supported" phrases.
5. **Non-local `VAR_REF`.** Globals and function names are rejected as `global variable`.
6. **Empty function bodies** were rejected in stage 181. They are now accepted and fall through to `mov w0, #0`.
7. **Unsigned literals and character literals** remain rejected (Phase 7).

## Implementation Plan

Order:

1. Tokenizer: none.
2. Parser: none.
3. AST: none.
4. Codegen: `include/codegen_arm64.h`, `src/codegen_arm64.c`
5. Tests: manifest tier-2 section, four new valid tests, integration test update
6. `src/version.c`: `VERSION_STAGE "01820000"`
7. Docs: milestone, transcript, checklist, README
8. Commit (only when the user asks)
9. Self-host: macOS C0→C1→C2; record Linux self-host as pending

Pause after each step for review.

## Notes

- Out of scope: everything in "Still rejected", register allocation or optimization (ARM64 `-O2` stays a no-op), `unsigned` arithmetic (`udiv`, `lsr`, Phase 7), and an ARM64 `print_asm` suite.
- This kickoff is planning only. It makes no compiler source, test, runner, or manifest changes.
