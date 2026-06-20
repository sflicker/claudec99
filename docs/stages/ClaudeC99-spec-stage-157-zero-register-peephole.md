# ClaudeC99 Stage 157 — Zero-Register Peephole Pattern

## Goal

Implement the first peephole optimization pattern: the **zero-register idiom**.
Replace `mov REG64, 0` (and `mov REG32, 0`) with `xor REG32, REG32` for any
general-purpose register.  `xor eax, eax` is 2 bytes; `mov rax, 0` is 7 bytes
and `mov eax, 0` is 5 bytes.  On x86-64, writing a 32-bit register zero-extends
to the full 64-bit register, so the two forms are semantically equivalent.

This stage touches `src/peephole.c` and `include/peephole.h` (a new
`peephole_builtin_patterns` accessor) and `src/compiler.c` (pass the
built-in pattern table to `peephole_run_file`).  No changes to codegen or the
AST optimizer.

---

## Background

The stage-155 peephole infrastructure built the sliding-window engine but
registered no patterns; `peephole_run_file` was called with `NULL, 0`.  This
stage adds the first real entry to the pattern table.

The codegen emits two zero-move forms:

- `    mov rax, 0` — for `TYPE_LONG` / `TYPE_LONG_LONG` integer literals with
  value `0`, and for `TYPE_UNSIGNED_LONG_LONG` literals.  (7-byte encoding:
  REX.W prefix + MOV r/m64, imm32.)
- `    mov eax, 0` — for `TYPE_INT` and shorter integer literal `0`; also
  emitted directly by logical-operator codegen for the false path.
  (5-byte encoding: MOV r/m32, imm32.)

Both map cleanly to `xor eax, eax` (2-byte encoding: XOR r/m32, r/m32 with
ModRM byte for reg == r/m).

The same idiom applies to all other general-purpose registers that the codegen
may zero (`rcx`, `rdx`, `r8`–`r15`, etc.), so the matcher covers the full
64-bit GP register set.

Related prior work:

- **Stage 155** — peephole infrastructure: `PeepholePattern`, `peephole_apply`,
  `peephole_run_file`; `-O2` flag; no patterns registered.
- **Stage 156** — `-O1` correctness bug fix; peephole infrastructure unchanged.

---

## Pattern specification

| Source line (trimmed)  | Replacement line (same indentation) |
|------------------------|--------------------------------------|
| `mov rax, 0`           | `xor eax, eax`                       |
| `mov rbx, 0`           | `xor ebx, ebx`                       |
| `mov rcx, 0`           | `xor ecx, ecx`                       |
| `mov rdx, 0`           | `xor edx, edx`                       |
| `mov rsi, 0`           | `xor esi, esi`                       |
| `mov rdi, 0`           | `xor edi, edi`                       |
| `mov r8, 0`            | `xor r8d, r8d`                       |
| `mov r9, 0`            | `xor r9d, r9d`                       |
| `mov r10, 0`           | `xor r10d, r10d`                     |
| `mov r11, 0`           | `xor r11d, r11d`                     |
| `mov r12, 0`           | `xor r12d, r12d`                     |
| `mov r13, 0`           | `xor r13d, r13d`                     |
| `mov r14, 0`           | `xor r14d, r14d`                     |
| `mov r15, 0`           | `xor r15d, r15d`                     |
| `mov eax, 0`           | `xor eax, eax`                       |
| `mov ebx, 0`           | `xor ebx, ebx`                       |
| `mov ecx, 0`           | `xor ecx, ecx`                       |
| `mov edx, 0`           | `xor edx, edx`                       |
| `mov esi, 0`           | `xor esi, esi`                       |
| `mov edi, 0`           | `xor edi, edi`                       |

**Window size:** 1 (single-line pattern).

**Match rule:** strip the line's leading whitespace; the trimmed string must
equal `mov REG, 0` exactly (no trailing characters, no inline comment).

**Replacement rule:** emit a single line with the same leading whitespace as
the original, followed by `xor REG32, REG32`.

---

## Task 1 — `src/peephole.c`

### 1a — Register table

Add a static parallel string array just before the new matcher, after the
existing `write_lines` function:

```c
/* Parallel source/destination register tables for the zero-register idiom.
   Index i: src_reg[i] is the matched register; dst_reg[i] is its 32-bit peer
   to use in the xor replacement. */
static const char * const g_zr_src[] = {
    "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
    "eax", "ebx", "ecx", "edx", "esi", "edi",
    NULL
};
static const char * const g_zr_dst[] = {
    "eax", "ebx", "ecx", "edx", "esi", "edi",
    "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
    "eax", "ebx", "ecx", "edx", "esi", "edi",
    NULL
};
```

The two arrays are indexed in parallel; `g_zr_src[i]` and `g_zr_dst[i]` are
always a matched pair.

### 1b — Helper: `zr_find_reg`

```c
/*
 * zr_find_reg -- after stripping whitespace prefix, check whether line
 * starts with "mov " followed by an entry in g_zr_src followed by ", 0"
 * with nothing after.  Returns the table index on match, -1 otherwise.
 */
static int zr_find_reg(const char *line) {
    const char *p;
    int i;
    size_t rlen;

    /* Skip leading whitespace. */
    p = line;
    while (*p == ' ' || *p == '\t') p++;

    /* Must start with "mov ". */
    if (strncmp(p, "mov ", 4) != 0) return -1;
    p += 4;

    /* Try each known source register. */
    for (i = 0; g_zr_src[i] != NULL; i++) {
        rlen = strlen(g_zr_src[i]);
        if (strncmp(p, g_zr_src[i], rlen) == 0) {
            /* Must be followed by exactly ", 0" and nothing else. */
            if (strcmp(p + rlen, ", 0") == 0) return i;
        }
    }
    return -1;
}
```

### 1c — Matcher

```c
static int match_zero_reg(const char **win, int n) {
    (void)n;
    return zr_find_reg(win[0]) >= 0;
}
```

### 1d — Replacer

```c
static void replace_zero_reg(const char **win, int n,
                              char **out, int *out_count) {
    const char *p;
    char buf[64];
    int  idx;
    size_t prefix_len;

    (void)n;

    idx = zr_find_reg(win[0]);

    /* Measure leading whitespace to preserve indentation. */
    p = win[0];
    while (*p == ' ' || *p == '\t') p++;
    prefix_len = (size_t)(p - win[0]);

    /* Build "    xor REG32, REG32" — buf is local, then strdup'd. */
    if (prefix_len >= sizeof(buf) - 1) prefix_len = sizeof(buf) - 1;
    strncpy(buf, win[0], prefix_len);
    buf[prefix_len] = '\0';
    /* Append "xor REG32, REG32". */
    strncat(buf, "xor ", sizeof(buf) - strlen(buf) - 1);
    strncat(buf, g_zr_dst[idx], sizeof(buf) - strlen(buf) - 1);
    strncat(buf, ", ", sizeof(buf) - strlen(buf) - 1);
    strncat(buf, g_zr_dst[idx], sizeof(buf) - strlen(buf) - 1);

    out[0] = util_strdup(buf);
    *out_count = 1;
}
```

### 1e — Built-in pattern table and accessor

Add just before the existing `peephole_apply` function:

```c
static const PeepholePattern g_builtin_patterns[] = {
    { 1, match_zero_reg, replace_zero_reg }
};
#define N_BUILTIN_PATTERNS \
    (int)(sizeof(g_builtin_patterns) / sizeof(g_builtin_patterns[0]))

const PeepholePattern *peephole_builtin_patterns(int *n_pats) {
    *n_pats = N_BUILTIN_PATTERNS;
    return g_builtin_patterns;
}
```

### 1f — Update file-top comment

Add to the existing comment block at the top of `src/peephole.c`:

```
 * Stage 157: zero-register idiom -- `mov REG, 0` → `xor REG32, REG32`
 *            for all 64-bit and 32-bit GP registers; single-line window.
```

---

## Task 2 — `include/peephole.h`

Add the accessor declaration after the `peephole_run_file` declaration:

```c
/*
 * peephole_builtin_patterns -- return the built-in pattern table.
 *
 * Sets *n_pats to the number of entries and returns a pointer to the
 * static pattern array.  Pass this directly to peephole_run_file or
 * peephole_apply.
 */
const PeepholePattern *peephole_builtin_patterns(int *n_pats);
```

---

## Task 3 — `src/compiler.c`

In `compile_one_file`, replace the `peephole_run_file` call:

```c
    /* Before (Stage 155): */
    if (peephole_run_file(output_path, NULL, 0) != 0) {

    /* After (Stage 157): */
    {
        int n_pats;
        const PeepholePattern *pats = peephole_builtin_patterns(&n_pats);
        if (peephole_run_file(output_path, pats, n_pats) != 0) {
```

Note: the closing `}` and error-path cleanup remain unchanged; only the
argument expressions change.  The extra `int n_pats;` declaration must appear
at the top of its enclosing block — wrap in `{ }` as shown or hoist it to the
nearest enclosing block's declaration area, whichever the existing code style
uses.

---

## Bootstrap caveats

`src/peephole.c` must compile cleanly under the C0 compiler:

- No VLAs.
- No `//` single-line comments — use `/* */` only.
- All variable declarations at the top of each block before any executable
  statements (C89 style).
- `strncpy`, `strncat`, `strlen`, `strncmp`, `strcmp` are in `<string.h>`,
  already included.
- `util_strdup` is in `"util.h"`, already included.
- `const char * const g_zr_src[]` and `g_zr_dst[]` — the C0 compiler supports
  `const` qualifiers and static arrays; use the simple pattern already seen in
  `codegen.c`.
- The `buf[64]` size must accommodate the longest possible replacement:
  `"    xor r15d, r15d"` is 20 characters including 4-space indent — well
  within 64 bytes.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

All new integration tests use `-O2` in their `.cflags` file.  Existing tests
at all optimization levels must be unaffected.

### test_zero_reg_int

Verify that a function returning integer literal `0` produces the correct
result; the `mov eax, 0` in the function body must be replaced by
`xor eax, eax` without changing observable behavior:

```c
/* test/integration/test_zero_reg_int/test_zero_reg_int.c */
#include <stdio.h>
int zero(void) { return 0; }
int main(void) {
    printf("%d\n", zero());
    return 0;
}
```

`.cflags`: `-O2`
`.expected`: `0`

### test_zero_reg_long

Verify that a `long`-typed zero return is also handled (`mov rax, 0` →
`xor eax, eax`):

```c
/* test/integration/test_zero_reg_long/test_zero_reg_long.c */
#include <stdio.h>
long lzero(void) { return 0L; }
int main(void) {
    printf("%ld\n", lzero());
    return 0;
}
```

`.cflags`: `-O2`
`.expected`: `0`

### test_zero_reg_arithmetic

Verify correctness when zero appears in arithmetic context — the zero-register
idiom fires for the literal `0`, and the surrounding arithmetic must still
produce the correct result:

```c
/* test/integration/test_zero_reg_arithmetic/test_zero_reg_arithmetic.c */
#include <stdio.h>
int main(void) {
    int a = 0;
    int b = 0;
    int c = 5;
    printf("%d\n", a + b + c);
    return 0;
}
```

`.cflags`: `-O2`
`.expected`: `5`

### test_zero_reg_logical

Verify that the logical-operator false path (`mov eax, 0` emitted directly
by codegen for the result of `a && b` when false) is also replaced correctly:

```c
/* test/integration/test_zero_reg_logical/test_zero_reg_logical.c */
#include <stdio.h>
int main(void) {
    int a = 1;
    int b = 0;
    printf("%d %d\n", a && b, a || b);
    return 0;
}
```

`.cflags`: `-O2`
`.expected`: `0 1`

### Regression

Run the full test suite via `./run_tests.sh`.  All existing tests must
continue to pass.  In particular:

- `-O0` tests: peephole pass does not run; no change.
- `-O1` tests: peephole pass runs with the built-in pattern table; zero-move
  instructions that happen to appear in the AST-optimized output may be
  replaced, but observable behavior is unchanged.
- `-O2` tests from Stage 155: `test_peephole_noop*` tests must still pass
  (the zero-register pattern fires on any `mov REG, 0` lines but produces
  equivalent code).

---

## Implementation order

1. Add the register tables (`g_zr_src`, `g_zr_dst`), `zr_find_reg`, `match_zero_reg`,
   `replace_zero_reg`, pattern table, and `peephole_builtin_patterns` to
   `src/peephole.c` (Tasks 1a–1f).
2. Add `peephole_builtin_patterns` declaration to `include/peephole.h` (Task 2).
3. Update the `peephole_run_file` call in `src/compiler.c` to pass the built-in
   pattern table (Task 3).
4. Build: `cmake --build build`.
5. Smoke test:
   ```sh
   printf '#include <stdio.h>\nint main(void){int a=0;printf("%%d\\n",a);return 0;}\n' \
     > /tmp/zr.c
   ./build/ccompiler -O2 /tmp/zr.c -o /tmp/zr.asm
   grep "xor eax" /tmp/zr.asm   # must appear
   grep "mov.*eax.*0" /tmp/zr.asm  # must not appear for the zero literal
   nasm -f elf64 /tmp/zr.asm -o /tmp/zr.o && gcc /tmp/zr.o -o /tmp/zr && /tmp/zr
   ```
   Expected output: `0`
6. Add the four integration tests.
7. Run `./run_tests.sh` — all tests pass.
8. Run `./build.sh --mode=self-host` — C0→C1→C2 self-host passes.
9. Bump `VERSION_STAGE` to `"01570000"` in `src/version.c`.

---

## Out of scope — do NOT do these in this stage

- **Other peephole patterns** — no-op move elimination (`mov rax, rax`),
  push/pop pair collapse, dead-jump removal, load-after-store forwarding, etc.
  are subsequent stages; do not implement any other pattern here.
- **Changing codegen to emit `xor` directly** — the point of the peephole
  pass is that codegen stays a pure AST → NASM translator; the machine-level
  idiom lives in `peephole.c`.
- **Re-scanning after replacement** — the engine already skips re-scanning;
  this single-line pattern cannot generate output that would itself match,
  so no special handling is needed.
- **`-O1` gating of the pattern** — the peephole pass (and therefore this
  pattern) runs only at `-O2`; `-O1` enables the AST optimizer only.  This is
  unchanged from Stage 155.
- **Inline comments on matched lines** — lines with an inline NASM comment
  after `mov rax, 0` (e.g., `mov rax, 0   ; zero`) will not be matched
  because the `, 0` suffix check would fail.  The codegen does not emit such
  comments on instruction lines, so this is not a practical limitation.

---

## Docs (at stage close, after all tests pass)

- **`docs/outlines/checklist.md`** — mark the "Zero-register idiom: `mov rax, 0`
  → `xor eax, eax`" item as complete (`[x]`), annotating Stage 157; add a
  `## Stage 157` section after `## Stage 156`.
- Run the **`update-supplemental-docs`** skill to refresh snapshots.
- **`docs/self-compilation-report.md`** — record the stage-157 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "01570000"`).
