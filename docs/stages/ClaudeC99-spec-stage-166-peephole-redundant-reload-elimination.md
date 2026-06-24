# ClaudeC99 Stage 166 — Peephole: Redundant Reload Elimination

## Background

The peephole optimizer (Stage 155) uses a sliding-window engine over emitted
NASM text.  Stage 157 registered the zero-register idiom; Stage 164 added
no-op move elimination; Stage 165 added push/pop pair collapse.  All four
patterns run at `-O2`.

Code generation frequently stores a value to a stack slot and then immediately
reloads it into the same register:

```nasm
    mov [rbp - 48], eax
    mov eax, [rbp - 48]
```

After the store, `eax` already holds the value that was just written.  The
reload is architecturally redundant: it reads back the value the register
already contains, at the cost of a memory access.  Removing it shrinks the
text segment and eliminates an unnecessary load from the hot path.

The window adjacency guarantee means no instruction or label can appear between
the two matched lines, so the "no intervening store" condition is met
automatically by the 2-line window.

This stage adds the fourth built-in peephole pattern: when a register-to-stack
store is immediately followed by a stack-to-register reload of the same slot
into the same register, delete the reload.

---

## Goal

Add a redundant reload elimination pattern to `src/peephole.c`:

```
mov [rbp - N], REG          →   mov [rbp - N], REG
mov REG, [rbp - N]              (reload deleted)
```

The pattern fires when both lines use the same register name and the same
decimal offset N.  Indentation is stripped when matching.  The store line is
passed through unchanged; the reload line is deleted.

---

## Implementation — `src/peephole.c`

### Helper: `pp_parse_store_rbp`

Parses a line of the form `mov [rbp - N], REG` (leading whitespace ignored).
Writes the decimal offset string into `off` and the register name into `reg`.
Returns 1 on success, 0 on any parse failure.

```c
static int pp_parse_store_rbp(const char *line, char *reg, size_t reg_sz,
                               char *off, size_t off_sz) {
    const char *p;
    const char *ostart;
    const char *rstart;
    size_t      olen;
    size_t      rlen;

    p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "mov ", 4) != 0) return 0;
    p += 4;
    if (strncmp(p, "[rbp - ", 7) != 0) return 0;
    p += 7;
    ostart = p;
    while (*p >= '0' && *p <= '9') p++;
    olen = (size_t)(p - ostart);
    if (olen == 0 || olen >= off_sz) return 0;
    strncpy(off, ostart, olen);
    off[olen] = '\0';
    if (strncmp(p, "], ", 3) != 0) return 0;
    p += 3;
    rstart = p;
    while (*p != '\0') p++;
    rlen = (size_t)(p - rstart);
    if (rlen == 0 || rlen >= reg_sz) return 0;
    strncpy(reg, rstart, rlen);
    reg[rlen] = '\0';
    return 1;
}
```

### Helper: `pp_parse_reload_rbp`

Parses a line of the form `mov REG, [rbp - N]` (leading whitespace ignored).
Writes the register name into `reg` and the decimal offset string into `off`.
Returns 1 on success, 0 on any parse failure.

```c
static int pp_parse_reload_rbp(const char *line, char *reg, size_t reg_sz,
                                char *off, size_t off_sz) {
    const char *p;
    const char *rstart;
    const char *ostart;
    size_t      rlen;
    size_t      olen;

    p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "mov ", 4) != 0) return 0;
    p += 4;
    rstart = p;
    while (*p != '\0' && *p != ',') p++;
    rlen = (size_t)(p - rstart);
    if (rlen == 0 || rlen >= reg_sz) return 0;
    strncpy(reg, rstart, rlen);
    reg[rlen] = '\0';
    if (strncmp(p, ", [rbp - ", 9) != 0) return 0;
    p += 9;
    ostart = p;
    while (*p >= '0' && *p <= '9') p++;
    olen = (size_t)(p - ostart);
    if (olen == 0 || olen >= off_sz) return 0;
    strncpy(off, ostart, olen);
    off[olen] = '\0';
    if (*p != ']' || *(p + 1) != '\0') return 0;
    return 1;
}
```

### Matcher: `match_redundant_reload`

```c
static int match_redundant_reload(const char **win, int n) {
    char r0[32];
    char o0[16];
    char r1[32];
    char o1[16];

    (void)n;
    if (!pp_parse_store_rbp(win[0],  r0, sizeof(r0), o0, sizeof(o0))) return 0;
    if (!pp_parse_reload_rbp(win[1], r1, sizeof(r1), o1, sizeof(o1))) return 0;
    return (strcmp(r0, r1) == 0 && strcmp(o0, o1) == 0);
}
```

### Replacer: `replace_redundant_reload`

Keep the store line; delete the reload line.

```c
static void replace_redundant_reload(const char **win, int n,
                                     char **out, int *out_count) {
    (void)n;
    out[0]     = util_strdup(win[0]);
    *out_count = 1;
}
```

### Update `g_builtin_patterns`

Expand the static array from 3 to 4 entries and register the new pattern:

```c
static PeepholePattern g_builtin_patterns[4];

const PeepholePattern *peephole_builtin_patterns(int *n_pats) {
    if (!g_builtin_patterns_ready) {
        g_builtin_patterns[0].window_size = 1;
        g_builtin_patterns[0].matcher     = match_zero_reg;
        g_builtin_patterns[0].replacer    = replace_zero_reg;
        g_builtin_patterns[1].window_size = 1;
        g_builtin_patterns[1].matcher     = match_nop_move;
        g_builtin_patterns[1].replacer    = replace_nop_move;
        g_builtin_patterns[2].window_size = 2;
        g_builtin_patterns[2].matcher     = match_push_pop;
        g_builtin_patterns[2].replacer    = replace_push_pop;
        g_builtin_patterns[3].window_size = 2;
        g_builtin_patterns[3].matcher     = match_redundant_reload;
        g_builtin_patterns[3].replacer    = replace_redundant_reload;
        g_builtin_patterns_ready          = 1;
    }
    *n_pats = 4;
    return g_builtin_patterns;
}
```

Update the file-header comment to record Stage 166:

```c
 * Stage 166: redundant reload elimination -- `mov [rbp-N], REG` / `mov REG, [rbp-N]` -> keep store, drop reload.
```

### Bootstrap constraint

All variable declarations (`r0`, `o0`, `r1`, `o1`, `p`, `ostart`, `rstart`,
`olen`, `rlen`) must appear at the top of their enclosing block before any
executable statements.  No VLAs; no `//` comments.  `string.h` is already
available via `util.h`.

---

## Tests

### Integration test: `test/integration/test_peephole_redundant_reload/`

A portable test (no system headers required) that verifies semantic correctness
after the optimization fires at `-O2`.

**`test_peephole_redundant_reload.c`**

```c
int pass_through(int a) { int x = a; return x; }

int main(void) {
    int v = pass_through(42);
    if (v != 42) return 1;
    return 42;
}
```

`pass_through` is written to reliably produce the store-then-reload pattern
(an `int` parameter spilled to `[rbp - N]` and immediately reloaded).  The
optimizer must eliminate the reload without changing the return value.

**`test_peephole_redundant_reload.cflags`**

```
-O2
```

**`test_peephole_redundant_reload.status`**

```
42
```

### Unit-level check (manual, not automated)

After building, compile a representative file at `-O2` and confirm no adjacent
store/reload pairs of the targeted form survive:

```sh
./bin/cc99 -O2 -S -o /tmp/check.asm demos/pong.c 2>/dev/null
grep -n 'mov \[rbp - ' /tmp/check.asm | awk -F: '{print $1+1, $0}' | \
    while read next rest; do
        sed -n "${next}p" /tmp/check.asm | grep -q 'mov.*\[rbp - ' && echo "PAIR at line $next"
    done
echo "scan done"
```

Alternatively, compile the test file without optimization and observe the
store/reload pair, then at `-O2` confirm the reload is absent:

```sh
./bin/cc99 -O0 -S -o /tmp/before.asm /tmp/test_peephole_redundant_reload.c 2>/dev/null
./bin/cc99 -O2 -S -o /tmp/after.asm  /tmp/test_peephole_redundant_reload.c 2>/dev/null
diff /tmp/before.asm /tmp/after.asm
```

---

## Version

Update `src/version.c`:

```c
#define VERSION_STAGE  "01660000"
```

---

## Implementation order

1. Add `pp_parse_store_rbp` and `pp_parse_reload_rbp` helper functions to
   `src/peephole.c` (insert after the `replace_push_pop` block, before
   `g_builtin_patterns`).
2. Add `match_redundant_reload` and `replace_redundant_reload` immediately after
   the two new helpers.
3. Expand `g_builtin_patterns[3]` → `g_builtin_patterns[4]` and register the
   new entry; update `*n_pats` from 3 to 4.
4. Update the file-header comment to mention Stage 166.
5. Add `test/integration/test_peephole_redundant_reload/` with the three files
   above.
6. Run `./test/run_all_tests.sh` — all tests pass.
7. Run `./build.sh --mode=self-host` — C0→C1→C2 verified.
8. Bump `VERSION_STAGE` to `"01660000"` in `src/version.c`.
