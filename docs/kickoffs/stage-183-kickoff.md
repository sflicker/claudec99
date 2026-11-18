# Stage 183 Kickoff: ARM64 Comparisons and Control Flow (Phase 3)

## Derived Values

| Item | Value |
|------|-------|
| STAGE_LABEL | stage-183 |
| STAGE_DISPLAY | Stage 183 |
| Spec | `docs/stages/ClaudeC99-spec-stage-183-arm64-control-flow.md` |
| Backend changed | `include/codegen_arm64.h` (`CodeGenArm64` gains `int label_count`, `Vec jumps`, a switch stack, `Vec user_labels`), `src/codegen_arm64.c` |
| Manifest | `test/valid/arm64_tiers.txt`: new `# Tier 3 — Phase 3 (stage 183)` section, 366 `3 <path>.c` lines |
| Manifest split | 199 phase-2-only files (already passing), 163 phase-3 files, 4 new tests |
| New tests | `test/valid/comparisons/test_cmp_long_neg_vs_int__1.c`, `test/valid/control_flow/test_switch_large_case_values__3.c`, `test/valid/control_flow/test_continue_in_switch_in_loop__9.c`, `test/valid/declarations/test_many_locals_deep_slot__45.c` |
| Deferred tests | `control_flow/test_switch_long_discriminant__7.c`, `expressions/test_conditional_mixed_width__1.c` (defect 2, not added) |
| VERSION_STAGE | `01830000` (stage 182 set `01820000`) |
| Expected output | Native `TARGET=arm64-macos ARM64_TIER=3 ./test/valid/run_tests.sh`: 439 of 1297 in scope, 439/439 passing; `ARM64_TIER=2` still 73/73; `ARM64_TIER=1` still 5/5; Linux `.asm` byte-identical to stage 182 for all test sources; x86 output unchanged; default macOS suite 2174 (2170 + 4 new valid); macOS self-host C0→C1→C2 passes; Linux self-host recorded as pending |

## 1. Spec Summary

Stage 183 is Phase 3 of the ARM64/macOS roadmap. It extends the `arm64-macos` backend from straight-line integer code (Stage 182) to statement-level control flow. Stage 182 remains the baseline for everything not listed here.

Supported in this stage (in addition to Stage 182):

- **Comparisons:** `== != < <= > >=` on `int`/`long` operands, signed. The result is `int` 0 or 1.
- **Logical:** `&&`, `||` (short-circuit) and unary `!`. The result is `int` 0 or 1.
- **Conditional:** `c ? a : b`. Only the selected branch is evaluated. The result is `long` if either branch is `long`.
- **Selection:** `if`, `if`/`else`, `switch` with `case`/`default` (fallthrough, nested switches, statements before the first label).
- **Iteration:** `while`, `do`-`while`, `for`. Each `for` clause is optional. The init may be a declaration or declaration list scoped to the loop.
- **Jumps:** `break` (innermost loop or switch), `continue` (innermost loop, passing through enclosing switches), `goto` and labels (forward and backward, into and out of blocks).

Still rejected with `error: arm64-macos: <construct> not yet supported`: parameters and calls (Phase 4); globals, `static`/`extern` locals and string literals (Phase 5); pointers, arrays, structs, unions, `&`, `*`, `sizeof`, initializer lists, compound literals (Phase 6); `char`, `short`, `unsigned`, unsigned and character literals, casts (Phase 7); floating point (Phase 8); varargs (Phase 9).

The Stage 182 messages `comparison operator`, `logical operator` and `logical not operator` are removed, because those operators are now supported.

The x86 targets (`x86_64-linux`, `x86_64-macos`) must not change. Auto-detection is unchanged.

### Codegen design highlights

- **Labels:** internal labels are `L<n>_<kind>` (for example `L3_else`, `L7_loop_cond`, `L12_case`), where `<n>` comes from the TU-wide `CodeGenArm64.label_count`. A C identifier cannot start with a digit, so these cannot collide with `L<func>_epilogue`. User labels are `L<func>_usr_<name>`. Mach-O treats `L`-prefixed labels as assembler-local.
- **Width helper:** `arm64_expr_is_long` returns `int` for comparisons, `&&`, `||` and `!`. Stage 182 returned the operand width for these, which must change. A conditional is `long` if either branch is `long`.
- **Comparisons:** both operands evaluated at the common width via the stack-machine sequence; `cmp`; `cset w0, <cc>` (signed condition codes).
- **Logical operators:** `cbz`/`cbnz` on each operand at its own width (a `long` with only upper bits set is true). `!a` is `cmp rA, #0; cset w0, eq`.
- **Conditional operator:** each branch converted with `arm64_expr_as` before the join, so an `int` branch of a `long` result is sign-extended (`sxtw`).
- **Statements:** conditions branch on zero (`cbz`); `do`-`while` uses `cbnz` to the body. No fusion of comparisons into `b.cc`. Layouts for `if`, `while`, `do`, `for` are fixed by the spec.
- **Jump stack:** `Vec jumps` of `{ break_label, continue_label }`. Loops push `{n, n}`; a switch pushes `{n, -1}`. `break` uses the top entry; `continue` uses the innermost entry with `continue_label >= 0`. Both are single `b` instructions.
- **Switch:** collect every `AST_CASE_SECTION` and `AST_DEFAULT_SECTION` in the body, without descending into nested switches (same walk as x86 `collect_switch_labels`). Evaluate the discriminant into `w0`/`x0`. For each case in source order, get the value with `strtol(value, NULL, 0)`, truncate to 32 bits for an `int` discriminant, then `cmp r0, #v` if the value is in 0..4095, else load it into `w1`/`x1` with `arm64_emit_load_imm` and `cmp r0, r1`; then `b.eq L<m>_case`. Fall back to default or `L<n>_switch_end`. The discriminant is never spilled.
- **Goto:** pre-walk collects every `AST_LABEL_STATEMENT` per function. Duplicates report `error: duplicate label '<name>' in function '<func>'`; a `goto` to an unknown name reports `error: undefined label '<name>' in function '<func>'`. Both match x86.
- **Frame pre-scan:** descends every non-NULL child of every statement (if/else, loop bodies, `for` init, case/default bodies, labels, switch bodies), so declarations there get slots. Slots are assigned in child order, which must equal the emitter's visiting order.
- **Stack invariant:** `sp == x29 - frame_size` at every statement boundary. Expression temporaries are pushed and popped within one expression, so `break`, `continue`, `goto`, case dispatch and `return` are plain branches.
- **Memory:** new `CodeGenArm64` fields are freed in `codegen_arm64_free`. Each switch's inner `Vec` is freed when the switch ends. `label_count` is reset only in `codegen_arm64_init`. `user_labels` is cleared per function.

## 2. Required Tokenizer Changes

None.

## 3. Required Parser Changes

None. `docs/grammar.md` stays as is.

## 4. Required AST Changes

None. The backend consumes the existing AST. The `for` node always has four children, `[init, cond, update, body]`, and any of the first three may be NULL. A declaration or `AST_DECL_LIST` init goes through the declaration path; anything else is an expression.

## 5. Required Code Generation Changes

- `include/codegen_arm64.h`: add `int label_count`, `Vec jumps` (`Arm64Jump { break_label, continue_label }`), the switch stack (`Arm64Switch` with a `Vec` of `Arm64SwitchLabel { node, label }` and `default_label`), and `Vec user_labels`.
- `src/codegen_arm64.c`:
  - Width helper changes (comparisons, logical, `!`, conditional).
  - `arm64_emit_load_imm` gains a destination register argument.
  - Comparisons, logical operators, `!`, conditional operator.
  - `if`, `if`/`else`, `while`, `do`-`while`, `for` (init scoped to the loop), `break`, `continue`.
  - `switch`/`case`/`default` with linear compare dispatch.
  - `goto`, labels, duplicate and undefined label errors.
  - Frame pre-scan change (descend every non-NULL child).
  - Init and free for the new fields.
  - Remove the Stage 182 rejection messages `comparison operator`, `logical operator`, `logical not operator`.
- `src/version.c`: `VERSION_STAGE "01830000"`.
- No changes to `src/codegen.c`, the x86 peephole pass, or any tokenizer, parser, or AST file.

## 6. Test Plan

### Manifest

- `test/valid/arm64_tiers.txt`: add a `# Tier 3 — Phase 3 (stage 183)` section with 366 entries, one `3 <path>.c` line each. Directories are not listed.
- Phase-2-only set (199): files outside the Stage 182 manifest that already pass natively (`bitwise/` 28, `control_flow/` 5, `declarations/` 37, `enums/` 13, `functions/` 2, `preprocessor/` 110, `stdlib/` 2, `typedefs/` 2). The spec lists them file by file.
- Phase-3 set (163): files that need Phase 3 constructs, spread across `arithmetic/`, `bitwise/`, `casting/`, `comparisons/`, `control_flow/`, `declarations/`, `enums/`, `expressions/`, `misc/`, `preprocessor/`, `statements/`, `stdlib/`. The spec lists them file by file.
- Excluded on purpose (x86-only predefined macros): `preprocessor/test_pp_predef_x86_64_ifdef__42.c`, `preprocessor/test_target_platform_macros__42.c`.
- No runner changes. `ARM64_TIER` still defaults to `1`.
- Rule for unexpected failures: a tier-3 candidate that fails for a reason outside Phase 3 is removed from the manifest and listed, with its blocker, in the milestone. Scope is not widened to fix it.

### New tests (target-neutral, join tier 3)

- (a) `test/valid/comparisons/test_cmp_long_neg_vs_int__1.c`: `long a = -1; int b = 0; return a < b;` (expected 1). Common-width comparison; the `int` operand is sign-extended.
- (b) `test/valid/control_flow/test_switch_large_case_values__3.c`: `switch` on `100000` with `case -5`, `case 4096`, `case 100000`, `default`. Expected 3. Exercises case values outside the `cmp` immediate range.
- (c) `test/valid/control_flow/test_continue_in_switch_in_loop__9.c`: `continue` inside a switch targets the enclosing `for`. Expected 9 (1 + 3 + 5).
- (d) `test/valid/declarations/test_many_locals_deep_slot__45.c`: 34 `long` locals `v0`..`v33`; `return v33 + v32 - v20;` (expected 45). Slot for `v33` is 272 bytes below `x29`, which exercises the `sub x9, x29, #off` path.

### Deferred (not added in this stage)

These fail on x86 today because of defect 2 and are not added:

- `control_flow/test_switch_long_discriminant__7.c`
- `expressions/test_conditional_mixed_width__1.c`

The ARM64 design handles both cases correctly. They are added when the x86 defect fix lands.

### Verification

1. Native ARM64 (Apple Silicon): `TARGET=arm64-macos ARM64_TIER=3 ./test/valid/run_tests.sh` reports 439 of 1297 in scope and 439/439 passing. `ARM64_TIER=2` gives 73/73 and `ARM64_TIER=1` gives 5/5.
2. macOS default target: full portable suite passes at 2174.
3. Linux regression: generated `.asm` for every test source under `x86_64-linux` is byte-identical to stage 182, except the new tests.
4. macOS self-host: `./build.sh --mode=self-host` passes C0→C1→C2, including the expanded `src/codegen_arm64.c`.
5. Linux self-host: remains pending. Record the status in the milestone.

If any manifest entry is removed under the unexpected-failure rule, the counts above drop accordingly; record the change in the milestone.

## Findings and Decisions

Decisions made before implementation:

1. **Spec file lists verified.** The 199 phase-2 and 163 phase-3 entries all exist in `test/valid`, have no duplicates, and are not already in the manifest.
2. **Case values are pre-folded.** The parser already folds case labels to literal strings (for example `case -3` gives the CaseSection value `"-3"`), so `strtol(value, NULL, 0)` is sufficient.
3. **No backend re-check of break/continue placement.** `break` and `continue` outside a loop or switch are rejected by the parser, so the backend does not re-check.
4. **Two defect-2 tests deferred.** `test_switch_long_discriminant__7` and `test_conditional_mixed_width__1` are not added, per spec.
5. **Conditional branch conversion.** Each branch goes through `arm64_expr_as` before the join. This is the ARM64 behavior the spec requires and differs from the current x86 backend.
6. **No discriminant spill.** The switch dispatches directly from the register, unlike x86, which pushes it.

## Implementation Plan

Order:

1. Tokenizer: none.
2. Parser: none.
3. AST: none.
4. Codegen: `include/codegen_arm64.h`, then `src/codegen_arm64.c`.
5. Tests: four new valid tests, `test/valid/arm64_tiers.txt` tier-3 section.
6. `src/version.c`: `VERSION_STAGE "01830000"`.
7. Docs: README, checklist, milestone, transcript, self-compilation report.
8. Commit (only when the user asks).
9. Self-host: macOS C0→C1→C2; record Linux self-host as pending.

Pause after each step for review.

## Notes

- Out of scope: everything in "Still rejected", fusing comparisons into conditional branches, jump tables for `switch`, other optimization (ARM64 `-O2` stays a no-op), unsigned comparisons (`lo`/`hi`/`ls`/`hs`, Phase 7), and fixing the x86 defect 2 bugs (separate stage).
- This kickoff is planning only. It makes no compiler source, test, runner, or manifest changes.
