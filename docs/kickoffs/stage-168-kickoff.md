# Stage 168 Kickoff — Peephole: Dead-Jump Removal

## Summary of the spec

Stage 168 implements the sixth concrete peephole optimization pattern: **dead-jump removal**. This pattern detects when a `jmp LXX` instruction is immediately followed by the label `LXX:`, and removes the jump because execution falls through to the label regardless. The jump is a no-op and can be safely deleted while keeping the label. The peephole infrastructure from Stage 155 already provides the sliding-window matching engine; Stage 157 registered the zero-register idiom; Stage 164 added no-op move elimination; Stage 165 added push/pop pair collapse; Stage 166 added redundant reload elimination; Stage 167 added redundant store elimination. This stage extends `g_builtin_patterns` from 5 to 6 entries and adds the fourth 2-line window pattern. No changes to tokenizer, parser, AST, or codegen are needed.

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation changes

None. The peephole pass operates on the final NASM assembly text after codegen completes. Dead-jump removal is purely a post-codegen optimization.

## Implementation tasks

### 1. Add pattern implementation to `src/peephole.c`

- Implement `pp_parse_jmp_label(const char *line, char *buf, int bufsz)` helper function to parse `jmp LABEL` lines (leading whitespace optional), extract the target label name, and strip trailing whitespace. Returns 1 on success, 0 on failure. Add this after the `pp_parse_store_rbp` block.

- Implement `pp_parse_label_def(const char *line, char *buf, int bufsz)` helper function to parse `LABEL:` lines (no leading whitespace allowed), extract the label name (without the colon), and strip trailing whitespace. Returns 1 on success, 0 on failure. Add this after `pp_parse_jmp_label`.

- Implement `match_dead_jump(const char **win, int n)` matcher function to call both helpers on `win[0]` and `win[1]`, and return 1 if both parse successfully and the target names match, 0 otherwise. Window size is 2.

- Implement `replace_dead_jump(const char **win, int n, char **out, int *out_count)` replacer function to delete `win[0]` (the jump) and keep `win[1]` (the label). Output count is 1.

- Expand `static PeepholePattern g_builtin_patterns[5]` to `[6]`.

- Register the new pattern as entry `[5]` in `peephole_builtin_patterns()` with `window_size = 2`, `matcher = match_dead_jump`, and `replacer = replace_dead_jump`.

- Update `*n_pats` return from 5 to 6.

- Update file-header comment to record Stage 168.

### 2. Bootstrap validation

- Ensure `src/peephole.c` compiles under the C0 compiler:
  - C89-style variable declarations only (all variables at top of block before statements)
  - No VLAs or `//` comments (use `/* */` only)
  - No new headers needed (`string.h` already included via `util.h`)

### 3. Version bump

- Update `VERSION_STAGE` in `src/version.c` to `"01680000"`.

## Test plan

### Integration test: `test/integration/test_peephole_dead_jump/`

1. **test_peephole_dead_jump.c** — function that triggers dead-jump pattern
   - Input: function `classify(int x)` that returns 1 if x > 0, else 0; `main` calls classify(5) expecting 1, classify(0) expecting 0, and returns 42
   - Expected: codegen emits conditional branching that produces a `jmp LXX` immediately followed by `LXX:` label; peephole eliminates the dead jump
   - Expected exit code: `42`

2. **test_peephole_dead_jump.cflags** — compile with `-O2` to enable peephole

3. **test_peephole_dead_jump.status** — expect exit code `42`

### Verification (manual, not automated)

After building, compile at `-O0` and `-O2` and verify the dead jump is eliminated:

```sh
./bin/cc99 -O0 -S -o /tmp/before.asm test/integration/test_peephole_dead_jump/test_peephole_dead_jump.c 2>/dev/null
./bin/cc99 -O2 -S -o /tmp/after.asm  test/integration/test_peephole_dead_jump/test_peephole_dead_jump.c 2>/dev/null
diff /tmp/before.asm /tmp/after.asm
```

The diff should show at least one `jmp` line deleted (the dead jump) while the corresponding label remains in both files.

### Regression testing

- Full test suite (`./run_tests.sh`) must pass at all optimization levels
- Existing Stage 157, 164, 165, 166, and 167 pattern tests continue to pass
- All other peephole tests unaffected

### Bootstrap validation

- Run `./build.sh --mode=self-host` to confirm C0→C1→C2 self-compilation succeeds

## Out of scope

- **Other peephole patterns** — future stages will add more patterns
- **Changing codegen** — codegen remains a pure AST→NASM translator; peephole patterns are post-codegen
- **Re-scanning after replacement** — the sliding-window engine does not re-scan; single-line pattern output cannot match itself
- **Larger windows** — future stages may add 3-line or n-line patterns; this stage adds a fourth 2-line pattern only
