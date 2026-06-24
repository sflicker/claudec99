# Stage 167 Kickoff — Peephole: Redundant Store Elimination

## Summary of the spec

Stage 167 implements the fifth concrete peephole optimization pattern: **redundant store elimination**. This pattern detects when two consecutive stores write to the same stack slot `[rbp - N]` with no intervening load, and deletes the first (dead) store. The first store produces no observable effect because its value is overwritten before the slot is ever read; the sliding-window adjacency guarantee ensures no instruction, label, or branch target can appear between the two matched lines.

Code generation frequently produces this pattern when a local variable is initialized and then immediately overwritten. The peephole infrastructure from Stage 155 already provides the sliding-window matching engine; Stage 157 registered the zero-register idiom; Stage 164 added no-op move elimination; Stage 165 added push/pop pair collapse; Stage 166 added redundant reload elimination. This stage extends `g_builtin_patterns` from 4 to 5 entries and adds the third 2-line window pattern. No changes to tokenizer, parser, AST, or codegen are needed.

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation changes

None. The peephole pass operates on the final NASM assembly text after codegen completes. Redundant store elimination is purely a post-codegen optimization.

## Implementation tasks

### 1. Add pattern implementation to `src/peephole.c`

- Implement `match_redundant_store(const char **win, int n)` matcher function to verify both lines are stores to `[rbp - N]` with matching offsets. Reuses the existing `pp_parse_store_rbp` helper (added in Stage 166) to extract the register name and offset from each line. Returns 1 if both offsets match (register names may differ), 0 otherwise. Window size is 2.

- Implement `replace_redundant_store(const char **win, int n, char **out, int *out_count)` replacer function to keep only the second store line (win[1]) and delete the first (win[0]). Output count is 1.

- Expand `static PeepholePattern g_builtin_patterns[4]` to `[5]`.

- Register the new pattern as entry `[4]` in `peephole_builtin_patterns()` with `window_size = 2`, `matcher = match_redundant_store`, and `replacer = replace_redundant_store`.

- Update `*n_pats` return from 4 to 5.

- Update file-header comment to record Stage 167.

### 2. Bootstrap validation

- Ensure `src/peephole.c` compiles under the C0 compiler:
  - C89-style variable declarations only (all variables at top of block before statements)
  - No VLAs or `//` comments (use `/* */` only)
  - No new headers needed (`string.h` already included via `util.h`)

### 3. Version bump

- Update `VERSION_STAGE` in `src/version.c` to `"01670000"`.

## Test plan

### Integration test: `test/integration/test_peephole_redundant_store/`

1. **test_peephole_redundant_store.c** — function that triggers store-store pattern
   - Input: function `overwrite(int a, int b)` that initializes a local variable to `a` and then immediately overwrites it with `b`, returning the final value
   - Expected: codegen emits `mov [rbp - N], ...` for the initial store and `mov [rbp - N], ...` for the overwrite; peephole eliminates the first store
   - Expected exit code: `42`

2. **test_peephole_redundant_store.cflags** — compile with `-O2` to enable peephole

3. **test_peephole_redundant_store.status** — expect exit code `42`

### Verification (manual, not automated)

After building, compile at `-O2` and verify the targeted store pair is eliminated:

```sh
./bin/cc99 -O0 -S -o /tmp/before.asm test/integration/test_peephole_redundant_store/test_peephole_redundant_store.c 2>/dev/null
./bin/cc99 -O2 -S -o /tmp/after.asm  test/integration/test_peephole_redundant_store/test_peephole_redundant_store.c 2>/dev/null
diff /tmp/before.asm /tmp/after.asm
```

The diff should show the first store line deleted while the second store line remains.

### Regression testing

- Full test suite (`./run_tests.sh`) must pass at all optimization levels
- Existing Stage 157, 164, 165, and 166 pattern tests continue to pass
- All other peephole tests unaffected

### Bootstrap validation

- Run `./build.sh --mode=self-host` to confirm C0→C1→C2 self-compilation succeeds

## Out of scope

- **Other peephole patterns** — future stages will add more patterns
- **Changing codegen** — codegen remains a pure AST→NASM translator; peephole patterns are post-codegen
- **Re-scanning after replacement** — the sliding-window engine does not re-scan; single-line pattern output cannot match itself
- **Larger windows** — future stages may add 3-line or n-line patterns; this stage adds a third 2-line pattern only
