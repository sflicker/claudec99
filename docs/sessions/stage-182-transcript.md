# stage-182 Transcript: ARM64 Integer Arithmetic and Local Variables

## Summary

Stage 182 extends the native `arm64-macos` backend from a function body that is a single `return <integer literal>` to straight-line signed integer code. Supported constructs are `int`, `long`, and `long long` (64-bit) block-scope locals with or without initializers, nested blocks with shadowing, `return <expr>;`, expression statements, integer literals of any size, parentheses, binary `+ - * / %`, `& | ^`, `<< >>` (`>>` is `asr`), unary `-`, `~`, and `+`, assignment (which yields its value), prefix and postfix `++`/`--` on locals, and the comma operator. Functions must be parameterless and return `int` or `long`. Only `main` runs, since calls arrive in Phase 4.

Every expression leaves its result in `w0` (int) or `x0` (long). Binary operations use a stack-machine pattern (`str x0, [sp, #-16]!`, evaluate right, `mov x1, x0`, `ldr x0, [sp], #16`). Each declaration gets its own frame slot, sized and aligned by its type, and slots are never reused. The frame is rounded to 16 bytes and limited to 4095 bytes. Falling off the end of a function returns 0 through the shared epilogue label.

Unsupported constructs are rejected with `arm64-macos: <construct> not yet supported`. Stage 181's out-of-range literal error is removed, and large literals are materialized with `mov`, or with `movz` followed by `movk` for each non-zero 16-bit chunk. No tokenizer, parser, AST, or grammar changes were made. The x86 targets are unchanged.

## Plan

The kickoff (`docs/kickoffs/stage-182-kickoff.md`) set the order as codegen header and source, then manifest and tests, then `VERSION_STAGE`, then docs, then self-host. One implementation deviation came from the frame limit. The spec addresses slots as `[x29, #-off]`, but `ldur`/`stur` reach only -256. Slots deeper than 256 bytes are addressed through `sub x9, x29, #off`, so the 4095-byte frame limit actually works.

## Changes Made

### Step 1: Codegen Header

- `include/codegen_arm64.h`: added `Arm64Local {name, offset, is_long, scope_depth}`.
- `include/codegen_arm64.h`: `CodeGenArm64` gains `Vec locals`, `scope_depth`, `next_offset`, `frame_size`, `func_name`, and `ret_is_long`.

### Step 2: Codegen Source

- `src/codegen_arm64.c`: added pure helper `arm64_expr_is_long` (literal declared type, local width, usual arithmetic conversions, shifts take the left operand's type, comma takes the right operand's type).
- `src/codegen_arm64.c`: added `arm64_emit_load_imm`, which uses `mov` for 0..65535, otherwise `movz` plus `movk` per non-zero 16-bit chunk, at `w0` for two chunks or up to four chunks at `x0`.
- `src/codegen_arm64.c`: binary operators (`add`, `sub`, `mul`, `sdiv`, `sdiv`+`msub` for `%`, `and`, `orr`, `eor`, `lsl`, `asr`) use the stack-machine pattern. Unary `-` and `~` emit `neg` and `mvn`; unary `+` emits no code.
- `src/codegen_arm64.c`: conversions use `sxtw x0, w0` for int to long and `w0` truncation for long to int.
- `src/codegen_arm64.c`: frame pre-scan assigns each declaration its own slot (int 4 bytes, 4-aligned; long 8 bytes, 8-aligned), rounds the frame to 16 bytes, emits `sub sp` only when nonzero, and rejects frames above 4095 bytes with `arm64-macos: stack frame larger than 4095 bytes not yet supported`.
- `src/codegen_arm64.c`: slots at or within 256 bytes use `ldur`/`stur` with `[x29, #-off]`. Deeper slots use `sub x9, x29, #off`.
- `src/codegen_arm64.c`: scope stack with innermost-first lookup for name resolution.
- `src/codegen_arm64.c`: assignment handles both AST shapes (named form with `value` set, and general lvalue form with a `VAR_REF` lhs). Prefix and postfix `++`/`--` and the comma operator are implemented.
- `src/codegen_arm64.c`: `AST_DECL_LIST` is treated as a container of declarations, each with its own slot.
- `src/codegen_arm64.c`: `return` converts to the function return width and branches to `b L<func>_epilogue`. Function end emits `mov w0, #0`, then `L<func>_epilogue:` with `mov sp, x29`, `ldp x29, x30, [sp], #16`, and `ret`.
- `src/codegen_arm64.c`: rejection messages for comparison operators, logical operators, logical not, unsigned literals, globals, static locals, extern declarations, `<kind> type` (unsigned, char, short, pointer, array, struct, floating-point), `<kind> return type`, function parameters, and return without a value. Stage 181's out-of-range literal error is removed, and empty function bodies are accepted.

### Step 3: Version

- `src/version.c`: `VERSION_STAGE` set to `01820000`.

### Step 4: Manifest and Test Runner Config

- `test/valid/arm64_tiers.txt`: new `# Tier 2 — Phase 2 (stage 182)` section with 68 entries (64 existing tier-2 files plus 4 new). `test_octal_zero__0` stays at tier 1 and gets no second entry.
- No runner changes. `ARM64_TIER` still defaults to `1`.

### Step 5: Tests

- New `test/valid/arithmetic/test_large_literals__17.c`: `long x = 4886718345L; int y = 305419896; return (x - y) % 256;` (expected 17).
- New `test/valid/arithmetic/test_long_to_int_truncate__42.c`: `long big = 4294967338L; int t = big; return t;` (expected 42).
- New `test/valid/expressions/test_nested_block_shadow__42.c`: outer and inner `x` with shadowing (expected 42).
- New `test/valid/arithmetic/test_main_falls_off_end__0.c`: `int a = 5; a = a * 2;` with no return (expected 0).
- `test/integration/test_arm64_skeleton_asm/`: `big_literal.c` becomes a positive check (compiles, `.s` contains `movz` and `movk`). `local_var.c` is removed. `comparison.c` (`return 1 < 2;`) is added as the unsupported-construct case. `run_test.sh` updated to match.
- Chained-assignment filenames: the spec listed `test_chained_assignment_1__42` through `_5__4`, but the real files for `_3` and `_4` are `test_chained_assignment_3__10` and `test_chained_assignment_4__9`. The manifest uses the real names.

### Step 6: Docs and Checkpoints

- Added the Stage 182 section to `docs/outlines/checklist.md`, the milestone and transcript, and the README Through Stage 182 entry and `--target` note.
- Self-host checkpoints recorded for C0, C1, and C2; self-compilation report updated.

## Final Results

- **Build**: Successful. `VERSION_STAGE` is `01820000`. C0 `00.03.01820000.01323`, C1 `00.03.01820000.01324`, C2 `00.03.01820000.01325`.
- **Native ARM64 tier 2 (`TARGET=arm64-macos ARM64_TIER=2`)**: 73 of 1293 tests in scope, 73/73 pass. C2 also drove this run, 73/73.
- **Native ARM64 tier 1 (`ARM64_TIER=1`)**: 5/5 pass.
- **Portable (macOS, x86_64-macos under Rosetta 2)**: 2170/2170 pass (249 unit, 1293 valid, 261 invalid, 196 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Up from 2166 at stage 181.
- **Self-host (macOS ARM64)**: C0, C1, and C2 each pass 2170/2170. No source changes during bootstrap.
- **Regression**: Linux-target `.asm` is byte-identical to stage 181 for all 1706 test sources that produce output (0 differences).
- **Not run**: Linux x86-64 self-host (pending).

## Session Flow

1. Read the stage-182 spec, the kickoff, and the stage-181 milestone and transcript style references.
2. Implemented the `CodeGenArm64` header fields and the codegen source (type helper, constants, operators, frame, scopes, assignment, return and epilogue).
3. Updated the tier-2 manifest, added the four new valid tests, and updated the integration test.
4. Set `VERSION_STAGE` to `01820000`.
5. Built and ran native ARM64 tier 1 and tier 2, then the macOS portable suite to 2170/2170.
6. Ran the Linux `.asm` byte-identity regression against stage 181.
7. Ran the macOS ARM64 self-host cycle (C0, C1, C2), each stage 2170/2170.
8. Wrote the milestone, transcript, checklist, and README artifacts.

## Notes

- Unsigned return types cannot be rejected by the backend because the parser does not record `unsigned` on function return types. This is harmless because int to unsigned long conversion is the same sign extension.
- `long long` is treated as `long`, both 64-bit.
- Declaring multiple variables in one statement (`int a, b;`) produces `AST_DECL_LIST`, which the backend handles as a container of declarations. No tier-2 test uses it.
- Compile-error line numbers remain offset by about 5 lines in both backends (pre-existing, follow-up).
- macOS `ld` warnings remain and are harmless.
- Follow-ups: Linux x86-64 self-host verification; Phase 3 (control flow); Phase 4 (parameters and calls); Phase 7 (unsigned arithmetic).

## Commit

- Implementation: `301938f` (stage 182: ARM64 integer arithmetic and local variables, Phase 2).
- Spec: `4556edd` (docs: stage 182 spec).
- Self-host checkpoints: `082da5b` (C0), `5e1ce1b` (C1).
- Self-host report: `f65d7c5` (docs: stage 182 self-compilation report).
