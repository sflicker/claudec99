# Stage 166 Kickoff — Peephole: Redundant Reload Elimination

## Summary of the spec

Stage 166 implements the fourth concrete peephole optimization pattern: **redundant reload elimination**. This pattern detects when a register-to-stack store is immediately followed by a stack-to-register reload of the same slot into the same register, and deletes the reload. The store line is preserved because it may be needed for aliasing or side effects; the reload is redundant because the register already holds the value just written.

Code generation frequently produces this pattern when a value is stored to a stack slot and then immediately reloaded into the same register. The peephole infrastructure from Stage 155 already provides the sliding-window matching engine; Stage 157 registered the zero-register idiom; Stage 164 added no-op move elimination; Stage 165 added push/pop pair collapse. This stage extends `g_builtin_patterns` from 3 to 4 entries and introduces the second 2-line window pattern. No changes to tokenizer, parser, AST, or codegen are needed.

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required peephole/codegen changes

None. The peephole pass operates on the final NASM assembly text after codegen completes. Redundant reload elimination is purely a post-codegen optimization.

## Implementation tasks

### 1. Add pattern implementation to `src/peephole.c`

- Implement `pp_parse_store_rbp(const char *line, char *reg, size_t reg_sz, char *off, size_t off_sz)` helper function to parse a line of the form `mov [rbp - N], REG`. Extracts the register name into `reg` and the decimal offset string into `off`. Returns 1 on success, 0 on parse failure. Handles leading whitespace.

- Implement `pp_parse_reload_rbp(const char *line, char *reg, size_t reg_sz, char *off, size_t off_sz)` helper function to parse a line of the form `mov REG, [rbp - N]`. Extracts the register name into `reg` and the decimal offset string into `off`. Returns 1 on success, 0 on parse failure. Handles leading whitespace.

- Implement `match_redundant_reload(const char **win, int n)` matcher function to verify the first line is a store to `[rbp - N]` and the second line is a reload from `[rbp - N]` into the same register. Returns 1 if both register names and offsets match, 0 otherwise. Window size is 2.

- Implement `replace_redundant_reload(const char **win, int n, char **out, int *out_count)` replacer function to keep only the store line (win[0]) and delete the reload line (win[1]). Output count is 1.

- Expand `static PeepholePattern g_builtin_patterns[3]` to `[4]`.

- Register the new pattern as entry `[3]` in `peephole_builtin_patterns()` with `window_size = 2`, `matcher = match_redundant_reload`, and `replacer = replace_redundant_reload`.

- Update `*n_pats` return from 3 to 4.

- Update file-header comment to record Stage 166.

### 2. Bootstrap validation

- Ensure `src/peephole.c` compiles under the C0 compiler:
  - C89-style variable declarations only (all variables at top of block before statements)
  - No VLAs or `//` comments (use `/* */` only)
  - No new headers needed (`string.h` already included via `util.h`)

### 3. Version bump

- Update `VERSION_STAGE` in `src/version.c` to `"01660000"`.

## Test plan

### Integration test: `test/integration/test_peephole_redundant_reload/`

1. **test_peephole_redundant_reload.c** — function that triggers store-reload pattern
   - Input: function `pass_through(int a)` that returns its parameter after spilling to a local variable
   - Expected: codegen emits `mov [rbp - N], eax` followed by `mov eax, [rbp - N]`; peephole eliminates the reload
   - Expected exit code: `42`

2. **test_peephole_redundant_reload.cflags** — compile with `-O2` to enable peephole

3. **test_peephole_redundant_reload.status** — expect exit code `42`

### Verification (manual, not automated)

After building, compile at `-O2` and verify the targeted store/reload pair is eliminated:

```sh
./cc99 -O0 -S -o /tmp/before.asm test/integration/test_peephole_redundant_reload/test_peephole_redundant_reload.c 2>/dev/null
./cc99 -O2 -S -o /tmp/after.asm  test/integration/test_peephole_redundant_reload/test_peephole_redundant_reload.c 2>/dev/null
diff /tmp/before.asm /tmp/after.asm
```

The diff should show the reload line deleted while the store line remains.

### Regression testing

- Full test suite (`./run_tests.sh`) must pass at all optimization levels
- Existing Stage 157, 164, and 165 pattern tests continue to pass
- All other peephole tests unaffected

### Bootstrap validation

- Run `./build.sh --mode=self-host` to confirm C0→C1→C2 self-compilation succeeds

## Out of scope

- **Other peephole patterns** — future stages will add more patterns
- **Changing codegen** — codegen remains a pure AST→NASM translator; peephole patterns are post-codegen
- **Re-scanning after replacement** — the sliding-window engine does not re-scan; single-line pattern output cannot match itself
- **Larger windows** — future stages may add 3-line or n-line patterns; this stage adds a second 2-line pattern only
