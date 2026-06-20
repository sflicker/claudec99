# Stage 157 Kickoff — Zero-Register Peephole Pattern

## Summary of the spec

Stage 157 implements the first concrete peephole optimization pattern: the **zero-register idiom**. This pattern replaces `mov REG, 0` and `mov REG32, 0` instructions (emitted by codegen for integer literal zero) with the more compact `xor REG32, REG32` instruction. The idiom works on all 64-bit and 32-bit general-purpose registers; since writing to a 32-bit register zero-extends to the full 64-bit register on x86-64, `xor eax, eax` (2 bytes) is a semantically equivalent replacement for `mov rax, 0` (7 bytes) or `mov eax, 0` (5 bytes).

The peephole infrastructure from Stage 155 already provides the sliding-window matching engine; this stage supplies the first pattern table entry via `peephole_builtin_patterns`, a new accessor function. No changes to tokenizer, parser, AST, or codegen are needed.

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation changes

None. The peephole pass operates on the final NASM assembly text after codegen completes. The zero-register pattern is purely a post-codegen optimization.

## Implementation tasks

### 1. Add register tables and pattern implementation to `src/peephole.c`

- Add `g_zr_src[]` and `g_zr_dst[]` static parallel register arrays mapping 64-bit and 32-bit GP registers to their 32-bit zero-extension targets.
- Implement `zr_find_reg()` helper to parse and match `mov REG, 0` lines.
- Implement `match_zero_reg()` matcher function (window size 1).
- Implement `replace_zero_reg()` replacer function to generate `xor REG32, REG32` with preserved indentation.
- Add `g_builtin_patterns[]` static pattern table containing the zero-register pattern.
- Implement `peephole_builtin_patterns()` accessor to return the built-in pattern table.
- Update file-top comment to note Stage 157 changes.

### 2. Add accessor declaration to `include/peephole.h`

- Declare `peephole_builtin_patterns(int *n_pats)` to expose the built-in pattern table.

### 3. Update `src/compiler.c`

- Replace the Stage 155 `peephole_run_file(output_path, NULL, 0)` call with a call that retrieves and passes the built-in patterns via `peephole_builtin_patterns()`.

### 4. Bootstrap validation

- Ensure `src/peephole.c` compiles under the C0 compiler:
  - C89-style variable declarations only (no VLAs or mixed code/declarations)
  - No `//` comments (use `/* */` only)
  - All standard string/memory functions already included
  - `util_strdup` available in `"util.h"`
  - Static `const` arrays supported by C0

### 5. Version bump

- Update `VERSION_STAGE` in `src/version.c` to `"01570000"`.

## Test plan

### Integration tests (all with `-O2` optimization)

1. **test_zero_reg_int** — integer literal zero return
   - Verifies `mov eax, 0` is replaced by `xor eax, eax`
   - Input: function returning `0`
   - Expected output: `0`

2. **test_zero_reg_long** — long integer literal zero return
   - Verifies `mov rax, 0` is replaced by `xor eax, eax`
   - Input: function returning `0L`
   - Expected output: `0`

3. **test_zero_reg_arithmetic** — zero in arithmetic context
   - Verifies pattern fires correctly and surrounding operations remain correct
   - Input: `int a = 0; int b = 0; int c = 5; a + b + c`
   - Expected output: `5`

4. **test_zero_reg_logical** — zero from logical operator codegen
   - Verifies the false-path `mov eax, 0` in logical AND/OR is replaced correctly
   - Input: `a && b` where `a = 1, b = 0`; `a || b`
   - Expected output: `0 1`

### Regression testing

- Full test suite (`./run_tests.sh`) must pass at all optimization levels
- Existing `-O0` and `-O1` tests unaffected (peephole does not run at `-O0`, runs with empty pattern table at `-O1`)
- Stage 155 peephole no-op tests (`test_peephole_noop*`) continue to pass
- Smoke test: compile a simple program with `-O2`, verify `xor eax, eax` appears in assembly and output is correct

### Bootstrap validation

- Run `./build.sh --mode=self-host` to confirm C0→C1→C2 self-compilation succeeds

## Out of scope

- **Other peephole patterns** — no-op move elimination, push/pop collapse, dead-jump removal, etc. are future stages
- **Changing codegen** — codegen remains a pure AST→NASM translator; the machine idiom lives in peephole
- **Re-scanning after replacement** — the sliding-window engine does not re-scan; single-line pattern output cannot match itself
- **`-O1` pattern gating** — peephole patterns run only at `-O2`; `-O1` enables AST optimization only
- **Inline comment matching** — lines with trailing NASM comments (e.g., `mov rax, 0   ; zero`) will not match due to the exact suffix check; codegen does not emit such comments on instruction lines
