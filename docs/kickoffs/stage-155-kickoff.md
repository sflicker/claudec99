# Stage 155 Kickoff — Peephole Optimizer Infrastructure

## Summary of the spec

Stage 155 establishes the post-codegen peephole optimizer infrastructure. The peephole pass:

- Reads the final NASM assembly file emitted by codegen
- Applies a sliding-window pattern matching engine (window sizes 2–4 lines)
- Writes the result back to the file

At this stage, no patterns are implemented; the pass is a transparent no-op. The infrastructure enables future stages to register individual patterns (zero-register idiom, no-op move elimination, push/pop collapse, etc.) by providing `matcher` and `replacer` function pointers.

The `-O2` flag is introduced as a superset of `-O1`: at `-O2`, both the AST-level optimizer and the peephole pass run. Invocation is in `compile_one_file` after codegen completes: `peephole_run_file(output_path, NULL, 0)`.

No structural changes to the compiler's front-end are required. The peephole layer cleanly separates machine-level cleanup from AST-level optimization.

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation changes

None. The peephole pass operates on the final text output after `codegen_translation_unit` completes.

## Implementation tasks

### 1. Create `include/peephole.h`

Define the pattern interface and sliding-window API:

- `PEEPHOLE_WINDOW_MAX` — maximum window size (4 lines)
- `PeepholeMatcher` — function pointer: `(const char * const *win, int n) → int` (1 if match)
- `PeepholeReplacer` — function pointer: `(const char * const *win, int n, char **out, int *out_count) → void`
- `PeepholePattern` — struct holding window size, matcher, and replacer
- `peephole_apply(char ***lines, int *nlines, const PeepholePattern *patterns, int n_pats)` — sliding-window engine
- `peephole_run_file(const char *path, const PeepholePattern *patterns, int n_pats)` — read file, apply patterns, write back

### 2. Create `src/peephole.c`

Implement four utility functions plus the main engine:

- **`read_lines`** (static) — read a file into a heap-allocated array of trimmed strings (no trailing newlines); return 0 on success, 1 on I/O failure
- **`write_lines`** (static) — write line array back to file with trailing newlines; return 0 on success, 1 on I/O failure
- **`peephole_apply`** — sliding-window engine (preferred variant: rebuild array in-place for each match; no re-scan after replacement)
- **`peephole_run_file`** — orchestrator: call `read_lines`, `peephole_apply`, and `write_lines`

Bootstrap constraints:
- C89-style declarations (all variables at block top, before statements)
- `/* */` comments only (no `//`)
- No VLAs
- Include `<stdio.h>`, `<stdlib.h>`, `<string.h>`, `"util.h"`, `"peephole.h"`

### 3. Update `src/compiler.c`

- Add `#include "peephole.h"` to the include block
- Parse `-O2` flag in `main`'s argument loop (after `-O1`)
- Update usage string to include `-O2`
- After `codegen_translation_unit` and `fclose(out)`, add:
  ```c
  if (opt_level >= 2) {
      if (peephole_run_file(output_path, NULL, 0) != 0) {
          /* error cleanup and return 1 */
      }
  }
  ```

### 4. Update `CMakeLists.txt`

Add `src/peephole.c` to the executable source list (after `src/optimize.c`).

### 5. Update `src/optimize.c` file-top comment

Add a note acknowledging Stage 155's peephole infrastructure (this file is unaffected).

## Test plan

### Integration tests (use `-O2` in `.cflags`)

1. **test_peephole_noop** — simple program with arithmetic; verify `-O2` produces correct output
   - Input: `int a = 1; int b = 2; printf("%d\n", a + b);`
   - Expected: `3`

2. **test_peephole_noop_loop** — loop with accumulation; verify no pattern fires and output is correct
   - Input: loop summing 1..5 into `s`
   - Expected: `15`

3. **test_peephole_noop_function** — multiple functions; verify `-O2` correctness across functions
   - Input: `add` and `mul` functions called from main
   - Expected: `7 12`

### Regression testing

- Run full test suite (`./run_tests.sh`) — all existing tests at `-O0`, `-O1`, `-O2` must pass
- Smoke test:
  ```sh
  printf '#include <stdio.h>\nint main(void){printf("hello\\n");return 0;}\n' > /tmp/ph.c
  ./build/ccompiler -O2 /tmp/ph.c -o /tmp/ph.asm
  nasm -f elf64 /tmp/ph.asm -o /tmp/ph.o && gcc /tmp/ph.o -o /tmp/ph && /tmp/ph
  ```
  Expected output: `hello`

### Bootstrap validation

- Run `./build.sh --mode=self-host` (C0→C1→C2) to confirm peephole.c compiles under C0

## Out of scope

- No peephole patterns (that is, all matching/replacement logic for specific idioms)
- No re-scanning of replacements
- No cross-function boundary awareness
- No `-O3` or IR optimizer
