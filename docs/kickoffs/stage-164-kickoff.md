# Stage 164 Kickoff — Peephole: No-op Move Elimination

## Summary of the spec

Stage 164 implements the second concrete peephole optimization pattern: **no-op move elimination**. This pattern deletes `mov REG, REG` instructions where the source and destination register names are identical. Self-moves are architecturally no-ops—they write the register back to itself, affecting no flags or state—and arise naturally when a value is already in the target register and the compiler does not track that fact.

The peephole infrastructure from Stage 155 already provides the sliding-window matching engine; Stage 157 registered the first pattern (zero-register idiom); this stage supplies the second pattern table entry. No changes to tokenizer, parser, AST, or codegen are needed.

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation changes

None. The peephole pass operates on the final NASM assembly text after codegen completes. No-op move elimination is purely a post-codegen optimization.

## Implementation tasks

### 1. Add pattern implementation to `src/peephole.c`

- Implement `match_nop_move(const char **win, int n)` matcher function to parse `mov DST, SRC` and check `DST == SRC` by string comparison; window size is 1.
- Implement `replace_nop_move(const char **win, int n, char **out, int *out_count)` replacer function to set `*out_count = 0` (delete the instruction).
- Expand `static PeepholePattern g_builtin_patterns[1]` to `[2]`.
- Register the new pattern as entry `[1]` in `peephole_builtin_patterns()`.
- Update `*n_pats` return from 1 to 2.
- Update file-header comment to record Stage 164.

### 2. Bootstrap validation

- Ensure `src/peephole.c` compiles under the C0 compiler:
  - C89-style variable declarations only (all variables at top of block before statements)
  - No VLAs or `//` comments (use `/* */` only)
  - No new headers needed (`string.h` already included via `util.h`)

### 3. Version bump

- Update `VERSION_STAGE` in `src/version.c` to `"01640000"`.

## Test plan

### Integration test: `test/integration/test_peephole_nop_move/`

1. **test_peephole_nop_move.c** — simple identity function
   - Input: function `identity(int x)` returning `x`; main calls it and verifies result
   - Expected: codegen may emit `mov rax, rax` or similar; peephole deletes it
   - Expected exit code: `42`

2. **test_peephole_nop_move.cflags** — compile with `-O2` to enable peephole

3. **test_peephole_nop_move.status** — expect exit code `42`

### Verification (manual, not automated)

After building, compile at `-O2` and grep for no-op self-moves:

```sh
./cc99 -O2 -S -o /tmp/check.asm demos/pong.c 2>/dev/null
grep -P '^\s+mov\s+(\w+),\s+\1$' /tmp/check.asm && echo "FOUND NOP MOVES" || echo "none"
```

### Regression testing

- Full test suite (`./run_tests.sh`) must pass at all optimization levels
- Existing Stage 157 zero-register tests continue to pass
- All other peephole tests unaffected

### Bootstrap validation

- Run `./build.sh --mode=self-host` to confirm C0→C1→C2 self-compilation succeeds

## Out of scope

- **Other peephole patterns** — future stages will add more patterns
- **Changing codegen** — codegen remains a pure AST→NASM translator; peephole patterns are post-codegen
- **Re-scanning after replacement** — the sliding-window engine does not re-scan; single-line pattern output cannot match itself
- **Multiple register sizes** — pattern matches all x86-64 register sizes (`rax`, `eax`, `ax`, `al`, extended registers `r8`–`r15`)
