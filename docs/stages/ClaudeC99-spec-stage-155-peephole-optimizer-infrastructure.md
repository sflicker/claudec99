# ClaudeC99 Stage 155 — Peephole Optimizer Infrastructure

## Goal

Establish the post-codegen peephole optimizer: `include/peephole.h` and
`src/peephole.c`.  The peephole pass reads the emitted NASM assembly file
line-by-line, slides a window of 2–4 consecutive lines across the text, and
applies registered pattern-matcher + replacer function pairs.

This stage adds the infrastructure only — no patterns are implemented.  At
`-O2`, the pipeline reads the output file, runs the (empty) pattern set, and
writes the file back unchanged.  Subsequent stages implement individual
patterns (zero-register idiom, no-op move elimination, push/pop collapse,
etc.) by registering them in the pattern table.

No changes to `src/codegen.c` are required or permitted by this stage.

---

## Background

The Optimize Level 1 AST optimizer (stages 142–154) rewrites constant
expressions and dead branches before codegen.  The Optimize Level 2 peephole
optimizer works in the opposite direction — it operates on the final text
output after codegen — and can collapse redundant instruction sequences that
are invisible at the AST level.

Because the pass reads and rewrites the emitted NASM file, it requires no
structural changes to `CodeGen`.  The cost is one extra read-write of the
output file at `-O2`; the benefit is a clean separation: codegen remains a
pure AST → NASM translation, and the peephole layer handles machine-level
cleanup.

`-O2` is a superset of `-O1`: when `opt_level >= 2`, both the AST-level
optimizer and the peephole pass run.  `-O1` continues to enable AST
optimization only.

Related prior work:

- **Stage 142** — AST optimizer skeleton; introduced `-O1` flag and
  `opt_level` parameter threading.
- **Stages 143–154** — AST-level optimization passes.

---

## Architecture

### Sliding window engine

The peephole pass represents the assembly output as an array of heap-allocated
strings — one per line, without trailing newlines.  A window of `w` lines
(where `w` is the `window_size` field of the pattern) slides forward one
position at a time.  At each position all registered patterns of that window
size are tried in order; the first matching pattern fires, its replacer
produces zero or more replacement lines, those lines are spliced into the
array, and scanning resumes immediately after the replacement (no re-scan).
If no pattern fires, the window advances by one line.

The scan is a single forward pass over the line array.  After the pass,
surviving lines are written to the output file with a trailing newline each.

### Pattern interface

```
matcher(win, n)         → 1 if window matches, 0 otherwise
replacer(win, n, out, out_count)
                        → fills out[0..] with replacement lines,
                          sets *out_count (0 means delete all n lines)
```

The replacer allocates each replacement line on the heap via `util_strdup` or
`malloc`.  The engine frees the original lines it replaces and takes ownership
of the replacement lines; the caller of `peephole_apply` owns the surviving
array.

### Pipeline change at `-O2`

In `compile_one_file` (in `src/compiler.c`), after `codegen_translation_unit`
and `fclose(out)`, add:

```
if (opt_level >= 2)
    peephole_run_file(output_path, NULL, 0);
```

At this stage `patterns = NULL, n_pats = 0`, so the pass reads the file,
makes no changes, and writes it back unchanged.  This validates the pipeline
without altering correctness.

---

## Task 1 — `include/peephole.h`

Create the new header:

```c
#ifndef CCOMPILER_PEEPHOLE_H
#define CCOMPILER_PEEPHOLE_H

/* Maximum number of consecutive lines in one peephole window. */
#define PEEPHOLE_WINDOW_MAX 4

/*
 * PeepholeMatcher — return 1 if the window of `n` lines matches.
 * `win` is a read-only array of `n` trimmed strings (no trailing newline).
 */
typedef int (*PeepholeMatcher)(const char * const *win, int n);

/*
 * PeepholeReplacer — write replacement lines for a matched window.
 * `out` is a caller-provided array of PEEPHOLE_WINDOW_MAX pointers.
 * The replacer fills out[0..*out_count-1] with heap-allocated strings
 * (no trailing newline) and sets *out_count (0 = delete all n lines).
 */
typedef void (*PeepholeReplacer)(const char * const *win, int n,
                                  char **out, int *out_count);

typedef struct {
    int              window_size; /* 1 to PEEPHOLE_WINDOW_MAX */
    PeepholeMatcher  matcher;
    PeepholeReplacer replacer;
} PeepholePattern;

/*
 * peephole_apply — run patterns over a mutable line array.
 *
 * *lines   : pointer to a heap-allocated array of heap-allocated strings
 *            (no trailing newlines).  On return, *lines and *nlines
 *            reflect the post-optimization state.  Replaced lines are
 *            freed; replacement lines are caller-owned heap strings.
 * *nlines  : line count; updated in place.
 * patterns : pattern table (may be NULL when n_pats == 0).
 * n_pats   : number of entries in patterns.
 *
 * Patterns are tried in registration order; the first match fires.
 * Scanning resumes after the replacement window (no re-scan).
 */
void peephole_apply(char ***lines, int *nlines,
                    const PeepholePattern *patterns, int n_pats);

/*
 * peephole_run_file — read path, apply patterns, write result back.
 *
 * Returns 0 on success, 1 on I/O failure (error printed to stderr).
 */
int peephole_run_file(const char *path,
                      const PeepholePattern *patterns, int n_pats);

#endif /* CCOMPILER_PEEPHOLE_H */
```

---

## Task 2 — `src/peephole.c`

Create the new source file.  The implementation has four parts:

### 2a — `read_lines`: read a file into a line array

```c
/*
 * Read all lines from `path` into a freshly allocated array of heap
 * strings (no trailing newlines).  Sets *out_lines and *out_count.
 * Returns 0 on success, 1 on failure.
 */
static int read_lines(const char *path, char ***out_lines, int *out_count) {
    FILE *f;
    char  buf[4096];
    int   cap, n;
    char **lines;

    f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "peephole: cannot open '%s' for reading\n", path);
        return 1;
    }

    cap   = 64;
    n     = 0;
    lines = malloc((size_t)cap * sizeof(char *));
    if (!lines) {
        fprintf(stderr, "peephole: out of memory\n");
        fclose(f);
        return 1;
    }

    while (fgets(buf, sizeof(buf), f)) {
        size_t len = strlen(buf);
        /* Strip trailing newline. */
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
            len--;
        buf[len] = '\0';

        if (n == cap) {
            char **grown;
            cap *= 2;
            grown = realloc(lines, (size_t)cap * sizeof(char *));
            if (!grown) {
                fprintf(stderr, "peephole: out of memory\n");
                /* Free already-allocated lines before returning. */
                while (n-- > 0) free(lines[n]);
                free(lines);
                fclose(f);
                return 1;
            }
            lines = grown;
        }
        lines[n] = util_strdup(buf);
        n++;
    }

    fclose(f);
    *out_lines = lines;
    *out_count = n;
    return 0;
}
```

### 2b — `write_lines`: write a line array back to a file

```c
/*
 * Write `n` lines to `path`, each followed by a newline.
 * Returns 0 on success, 1 on failure.
 */
static int write_lines(const char *path, char * const *lines, int n) {
    FILE *f;
    int   i;

    f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "peephole: cannot open '%s' for writing\n", path);
        return 1;
    }

    for (i = 0; i < n; i++) {
        if (fprintf(f, "%s\n", lines[i]) < 0) {
            fprintf(stderr, "peephole: write error on '%s'\n", path);
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}
```

### 2c — `peephole_apply`: the sliding window engine

```c
void peephole_apply(char ***lines, int *nlines,
                    const PeepholePattern *patterns, int n_pats) {
    int i, p, w, matched, out_count;
    char *out_buf[PEEPHOLE_WINDOW_MAX];
    char **arr;
    int   n;

    if (n_pats == 0 || patterns == NULL) return; /* nothing to do */

    arr = *lines;
    n   = *nlines;
    i   = 0;

    while (i < n) {
        matched = 0;
        for (p = 0; p < n_pats && !matched; p++) {
            w = patterns[p].window_size;
            if (i + w > n) continue;   /* window extends past end */

            if (patterns[p].matcher((const char * const *)(arr + i), w)) {
                int j, k, delta, new_n;
                char **new_arr;

                out_count = 0;
                patterns[p].replacer((const char * const *)(arr + i), w,
                                     out_buf, &out_count);

                /* delta: negative shrinks, positive grows, zero is same */
                delta = out_count - w;
                new_n = n + delta;

                if (delta != 0) {
                    new_arr = malloc((size_t)(new_n > 0 ? new_n : 1)
                                     * sizeof(char *));
                    if (!new_arr) {
                        fprintf(stderr, "peephole: out of memory\n");
                        /* Free any replacer output already allocated. */
                        for (k = 0; k < out_count; k++) free(out_buf[k]);
                        return; /* leave array unchanged */
                    }
                    /* Copy prefix [0 .. i-1] unchanged. */
                    for (k = 0; k < i; k++) new_arr[k] = arr[k];
                    /* Copy suffix [i+w .. n-1] shifted by delta. */
                    for (k = i + w; k < n; k++)
                        new_arr[k + delta] = arr[k];
                    free(arr);
                    arr = new_arr;
                    n   = new_n;
                    *lines  = arr;
                    *nlines = n;
                }

                /* Free the replaced window lines. */
                for (k = 0; k < w; k++) free(arr[i + (delta != 0 ? 0 : k)]);
                /* When delta != 0, arr has already been rebuilt; the old
                   window lines were not copied — they are freed above via
                   the pre-copy arr (now freed).  So we must free only via
                   out_buf[k] for the already-freed originals.  Simplify:
                   always free originals from the old array before realloc. */
                /* Note: this is restructured in the explicit code below. */

                /* Install replacement lines. */
                for (k = 0; k < out_count; k++) arr[i + k] = out_buf[k];

                matched = 1;
                i += out_count; /* skip past replacement output */
            }
        }
        if (!matched) i++;
    }
}
```

**Note:** The memory management in the sliding engine is the trickiest part
because the old window lines must be freed before or after the array is
rebuilt.  The recommended implementation below avoids the complexity by
always using a rebuild-from-scratch approach for the splice:

**Preferred implementation of `peephole_apply`** (clearer memory semantics):

```c
void peephole_apply(char ***lines, int *nlines,
                    const PeepholePattern *patterns, int n_pats) {
    int i, p;
    char *out_buf[PEEPHOLE_WINDOW_MAX];
    int   out_count;

    if (n_pats == 0 || patterns == NULL) return;

    i = 0;
    while (i < *nlines) {
        int matched = 0;
        for (p = 0; p < n_pats && !matched; p++) {
            int w = patterns[p].window_size;
            int j, k, new_n, delta;
            char **old_arr, **new_arr;

            if (i + w > *nlines) continue;

            if (!patterns[p].matcher(
                    (const char * const *)(*lines + i), w))
                continue;

            out_count = 0;
            patterns[p].replacer(
                (const char * const *)(*lines + i), w,
                out_buf, &out_count);

            /* Build a new array: prefix + replacements + suffix. */
            old_arr = *lines;
            delta   = out_count - w;
            new_n   = *nlines + delta;
            new_arr = malloc((size_t)(new_n > 0 ? new_n : 1) * sizeof(char *));
            if (!new_arr) {
                fprintf(stderr, "peephole: out of memory\n");
                for (k = 0; k < out_count; k++) free(out_buf[k]);
                return;
            }

            /* Copy prefix. */
            for (j = 0; j < i; j++) new_arr[j] = old_arr[j];
            /* Insert replacements. */
            for (k = 0; k < out_count; k++) new_arr[i + k] = out_buf[k];
            /* Copy suffix. */
            for (j = i + w; j < *nlines; j++)
                new_arr[j + delta] = old_arr[j];

            /* Free the replaced window lines. */
            for (k = 0; k < w; k++) free(old_arr[i + k]);
            free(old_arr);

            *lines  = new_arr;
            *nlines = new_n;

            matched = 1;
            i += out_count;
        }
        if (!matched) i++;
    }
}
```

### 2d — `peephole_run_file`

```c
int peephole_run_file(const char *path,
                      const PeepholePattern *patterns, int n_pats) {
    char **lines;
    int    n;

    if (read_lines(path, &lines, &n) != 0) return 1;
    peephole_apply(&lines, &n, patterns, n_pats);
    if (write_lines(path, (char * const *)lines, n) != 0) {
        int k;
        for (k = 0; k < n; k++) free(lines[k]);
        free(lines);
        return 1;
    }

    {
        int k;
        for (k = 0; k < n; k++) free(lines[k]);
        free(lines);
    }
    return 0;
}
```

### 2e — File-top comment block

```c
/*
 * peephole.c - Post-codegen peephole optimizer.
 *
 * Stage 155: infrastructure -- sliding window engine over the emitted
 *            NASM text; patterns expressed as matcher + replacer pairs.
 *            No patterns registered at this stage; the pass is a no-op.
 *
 * Activated at -O2 (implies -O1: AST optimizer also runs).
 */
```

---

## Task 3 — `src/compiler.c`

### 3a — Add `-O2` flag

In the argument-parsing loop in `main`, after the `-O1` branch:

```c
        } else if (strcmp(argv[i], "-O2") == 0) {
            opt_level = 2;
```

Also update the usage string to include `-O2`:

```
usage: ccompiler ... [-O0|-O1|-O2] ...
```

### 3b — Invoke peephole pass after codegen

In `compile_one_file`, after `fclose(out)` and before the `printf("compiled: ...")` line:

```c
    if (opt_level >= 2) {
        if (peephole_run_file(output_path, NULL, 0) != 0) {
            parser_free(&parser);
            lexer_free(&lexer);
            ast_free(ast);
            free(preprocessed);
            return 1;
        }
    }
```

### 3c — Include the new header

Add `#include "peephole.h"` to the include block in `compiler.c`.

---

## Task 4 — `CMakeLists.txt`

Add `src/peephole.c` to the `add_executable` source list (after `src/optimize.c`):

```cmake
    src/optimize.c
    src/peephole.c
```

---

## Task 5 — Update file-top comment in `optimize.c`

No functional change; add a note referencing the new stage so the optimizer
comment block tracks the overall optimization story:

```
 * Stage 155: peephole infrastructure added in peephole.c / peephole.h;
 *            this file (optimize.c) is unaffected.
```

---

## Bootstrap caveats

`src/peephole.c` must compile cleanly under the C0 compiler (previous-stage
binary):

- No VLAs.
- No `//` single-line comments — use `/* */` only.
- All variable declarations must appear at the top of each block, before any
  executable statements — C89 style.
- `util_strdup` is declared in `"util.h"`; include it.
- `malloc`, `realloc`, `free` are in `<stdlib.h>`.
- `fopen`, `fgets`, `fprintf`, `fclose` are in `<stdio.h>`.
- `strlen` is in `<string.h>`.
- `PEEPHOLE_WINDOW_MAX` is 4; no use of variable-length arrays.
- `PeepholeMatcher` and `PeepholeReplacer` are function pointer typedefs; the
  C0 compiler supports function pointers as used in codegen and optimize.
- The `matched` variable in `peephole_apply` is declared before the `while`
  loop, not inside it (C89 restriction).

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

All new integration tests use `-O2` in their `.cflags` file.  Existing tests
(including all `-O0` and `-O1` tests) must be unaffected.

### test_peephole_noop

Verify that `-O2` (with no active patterns) produces the same observable
output as `-O0` for a simple program:

```c
/* test/integration/test_peephole_noop/test_peephole_noop.c */
#include <stdio.h>
int main(void) {
    int a = 1;
    int b = 2;
    printf("%d\n", a + b);
    return 0;
}
```

`.cflags`: `-O2`
`.expected`: `3`

### test_peephole_noop_loop

Verify `-O2` correctness on a loop (no pattern fires, output is unchanged):

```c
/* test/integration/test_peephole_noop_loop/test_peephole_noop_loop.c */
#include <stdio.h>
int main(void) {
    int i;
    int s = 0;
    for (i = 1; i <= 5; i++) s += i;
    printf("%d\n", s);
    return 0;
}
```

`.cflags`: `-O2`
`.expected`: `15`

### test_peephole_noop_function

Verify `-O2` with multiple functions:

```c
/* test/integration/test_peephole_noop_function/test_peephole_noop_function.c */
#include <stdio.h>
int add(int x, int y) { return x + y; }
int mul(int x, int y) { return x * y; }
int main(void) {
    printf("%d %d\n", add(3, 4), mul(3, 4));
    return 0;
}
```

`.cflags`: `-O2`
`.expected`: `7 12`

### Regression

Run the full test suite via `./run_tests.sh`.  All existing tests must
continue to pass at `-O0`, `-O1`, and `-O2`.  The peephole infrastructure
is a transparent no-op at this stage, so no test should change behaviour.

---

## Implementation order

1. Create `include/peephole.h` with the type definitions and function
   declarations described in Task 1.
2. Create `src/peephole.c` with the file-top comment block (2e), then
   `read_lines` (2a), `write_lines` (2b), `peephole_apply` (2c preferred
   variant), and `peephole_run_file` (2d).  Include `<stdio.h>`,
   `<stdlib.h>`, `<string.h>`, `"util.h"`, and `"peephole.h"`.
3. Add `src/peephole.c` to `CMakeLists.txt` (Task 4).
4. In `compiler.c`: add `#include "peephole.h"`, parse `-O2`, invoke
   `peephole_run_file` after codegen (Task 3).
5. Update the file-top comment in `src/optimize.c` (Task 5).
6. Build: `cmake --build build`.
7. Smoke test:
   ```sh
   printf '#include <stdio.h>\nint main(void){printf("hello\\n");return 0;}\n' \
     > /tmp/ph.c
   ./build/ccompiler -O2 /tmp/ph.c -o /tmp/ph.asm
   nasm -f elf64 /tmp/ph.asm -o /tmp/ph.o && gcc /tmp/ph.o -o /tmp/ph && /tmp/ph
   ```
   Expected output: `hello`
8. Add the three integration tests.
9. Run `./run_tests.sh` — all tests pass.
10. Run `./build.sh --mode=self-host` — C0→C1→C2 self-host passes.
11. Bump `VERSION_STAGE` to `"01550000"` in `src/version.c`.

---

## Out of scope — do NOT do these in this stage

- **Any peephole patterns** — zero-register idiom, no-op move elimination,
  push/pop collapse, load/store elimination, dead-jump removal, etc. are all
  subsequent stages; do not implement any pattern here.
- **Re-scan after replacement** — patterns that generate output which itself
  could match another pattern are not handled; scanning always resumes after
  the replacement window.
- **Cross-function windows** — NASM label boundaries are not considered;
  the window may span function prologues only if they happen to be adjacent
  lines, which is harmless since no patterns exist yet.
- **`-O3` or IR optimizer** — the checklist's Optimize Level 3 (IR-based
  optimizations) is a separate multi-stage effort; this stage introduces
  `-O2` for the peephole level only.
- **`open_memstream` or in-process buffering** — the file-read-write approach
  is sufficient and avoids POSIX extension dependencies in codegen.

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the "Infrastructure: `peephole.c`
  / `include/peephole.h`; sliding window (2–4 lines) over the output buffer;
  patterns expressed as matcher + replacer functions" item as complete
  (`[x]`), annotating Stage 155; add a `## Stage 155` section after
  `## Stage 154`.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-155 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01550000"`).
