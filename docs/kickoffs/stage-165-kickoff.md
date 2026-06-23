# Stage 165 Kickoff — Peephole: Push/Pop Pair Collapse

## Summary of the spec

Stage 165 implements the third concrete peephole optimization pattern: **push/pop pair collapse**. This pattern transforms adjacent `push rX` / `pop rY` instruction pairs into a single `mov rY, rX`, or deletes both lines if the registers are identical. Consecutive push/pop pairs arise naturally when a value is materialized on the stack and immediately consumed; collapsing them into register moves eliminates memory traffic and reduces text-segment size.

The peephole infrastructure from Stage 155 already provides the sliding-window matching engine; Stage 157 registered the zero-register idiom; Stage 164 added no-op move elimination. This stage extends `g_builtin_patterns` from 2 to 3 entries and introduces the first 2-line window pattern. No changes to tokenizer, parser, AST, or codegen are needed.

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation changes

None. The peephole pass operates on the final NASM assembly text after codegen completes. Push/pop pair collapse is purely a post-codegen optimization.

## Implementation tasks

### 1. Add pattern implementation to `src/peephole.c`

- Implement `pp_extract_reg(const char *line, const char *mnemonic, char *dst, size_t dst_size)` helper function to extract the register name from a `push` or `pop` line. Returns 1 on success, 0 on parse failure. Accepts expected mnemonic prefix (`"push "` or `"pop "`).
- Implement `match_push_pop(const char **win, int n)` matcher function to verify both lines are valid push/pop with any register names; window size is 2.
- Implement `replace_push_pop(const char **win, int n, char **out, int *out_count)` replacer function to:
  - Extract registers from both lines
  - If registers are identical, delete both lines (`*out_count = 0`)
  - Otherwise, emit a single `mov rY, rX` line (preserving the leading whitespace from the push line)
- Expand `static PeepholePattern g_builtin_patterns[2]` to `[3]`.
- Register the new pattern as entry `[2]` in `peephole_builtin_patterns()` with `window_size = 2`.
- Update `*n_pats` return from 2 to 3.
- Update file-header comment to record Stage 165.

### 2. Bootstrap validation

- Ensure `src/peephole.c` compiles under the C0 compiler:
  - C89-style variable declarations only (all variables at top of block before statements)
  - No VLAs or `//` comments (use `/* */` only)
  - No new headers needed (`string.h` already included via `util.h`)

### 3. Version bump

- Update `VERSION_STAGE` in `src/version.c` to `"01650000"`.

## Test plan

### Integration test: `test/integration/test_peephole_push_pop/`

1. **test_peephole_push_pop.c** — function with temporary value passed through stack
   - Input: function `add(int a, int b)` returning `a + b`; main calls it twice in sequence and verifies results
   - Expected: codegen emits push/pop pairs for parameter passing; peephole collapses them
   - Expected exit code: `42`

2. **test_peephole_push_pop.cflags** — compile with `-O2` to enable peephole

3. **test_peephole_push_pop.status** — expect exit code `42`

### Verification (manual, not automated)

After building, compile at `-O2` and verify no naked push/pop pairs remain:

```sh
./cc99 -O2 -S -o /tmp/check.asm demos/pong.c 2>/dev/null
grep -A1 '^\s*push ' /tmp/check.asm | grep '^\s*pop ' && echo "PAIRS FOUND" || echo "none"
```

### Regression testing

- Full test suite (`./run_tests.sh`) must pass at all optimization levels
- Existing Stage 157 and 164 pattern tests continue to pass
- All other peephole tests unaffected

### Bootstrap validation

- Run `./build.sh --mode=self-host` to confirm C0→C1→C2 self-compilation succeeds

## Out of scope

- **Other peephole patterns** — future stages will add more patterns
- **Changing codegen** — codegen remains a pure AST→NASM translator; peephole patterns are post-codegen
- **Re-scanning after replacement** — the sliding-window engine does not re-scan; single-line pattern output cannot match itself
- **Larger windows** — future stages may add 3-line or n-line patterns; this stage adds the first 2-line pattern only
