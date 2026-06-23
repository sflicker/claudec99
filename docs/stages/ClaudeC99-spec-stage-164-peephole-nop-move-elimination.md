# ClaudeC99 Stage 164 — Peephole: No-op Move Elimination

## Background

The peephole optimizer introduced in Stage 155 uses a sliding-window engine over
emitted NASM text.  Stage 157 registered the first pattern: `mov REG, 0` →
`xor REG32, REG32`.  The optimizer runs at `-O2`.

Code generation occasionally emits self-moves — `mov rax, rax` — that are
architecturally no-ops: they write the register back to itself, affecting no
flags and no state.  These arise naturally when a value is already in the target
register and the compiler does not track that fact.  Removing them shrinks the
text segment without any semantic change.

This stage adds the second built-in peephole pattern: delete any single-line
`mov REG, REG` instruction where the source and destination register names are
identical.

---

## Goal

Add a no-op move elimination pattern to `src/peephole.c`:

```
mov REG, REG   (same register name on both sides)   →   (deleted)
```

The pattern fires on all register sizes (64-bit `rax`, 32-bit `eax`, 16-bit
`ax`, 8-bit `al`, extended registers `r8`–`r15` in all widths).  Indentation
is ignored during matching; the entire line is deleted from the output.

---

## Implementation — `src/peephole.c`

### Matcher: `match_nop_move`

Scan the line for `mov DST, SRC` where DST and SRC are the same token.

```c
static int match_nop_move(const char **win, int n) {
    const char *p;
    const char *dst_start;
    size_t dst_len;

    (void)n;
    p = win[0];
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "mov ", 4) != 0) return 0;
    p += 4;

    dst_start = p;
    while (*p != '\0' && *p != ',') p++;
    if (*p != ',') return 0;
    dst_len = (size_t)(p - dst_start);
    p++; /* skip ',' */
    if (*p != ' ') return 0;
    p++; /* skip ' ' */

    /* Match if SRC == DST and nothing follows. */
    return (strncmp(p, dst_start, dst_len) == 0 && p[dst_len] == '\0');
}
```

### Replacer: `replace_nop_move`

Output zero lines (delete the instruction):

```c
static void replace_nop_move(const char **win, int n,
                              char **out, int *out_count) {
    (void)win;
    (void)n;
    (void)out;
    *out_count = 0;
}
```

### Update `g_builtin_patterns`

Expand the static array from 1 to 2 entries and register the new pattern:

```c
static PeepholePattern g_builtin_patterns[2];

const PeepholePattern *peephole_builtin_patterns(int *n_pats) {
    if (!g_builtin_patterns_ready) {
        g_builtin_patterns[0].window_size = 1;
        g_builtin_patterns[0].matcher     = match_zero_reg;
        g_builtin_patterns[0].replacer    = replace_zero_reg;
        g_builtin_patterns[1].window_size = 1;
        g_builtin_patterns[1].matcher     = match_nop_move;
        g_builtin_patterns[1].replacer    = replace_nop_move;
        g_builtin_patterns_ready          = 1;
    }
    *n_pats = 2;
    return g_builtin_patterns;
}
```

Update the file-header comment to record Stage 164:

```c
 * Stage 164: no-op move elimination -- `mov REG, REG` (same src/dst) -> remove.
```

### Bootstrap constraint

All new variable declarations (`dst_start`, `dst_len`) must appear at the top
of their block before any executable statements.  No VLAs; no `//` comments.
No new headers needed (`string.h` is already included via `util.h`).

---

## Tests

### Integration test: `test/integration/test_peephole_nop_move/`

Add a portable test (no system headers required) that exercises the pattern
end-to-end via the compiler at `-O2`.

**`test_peephole_nop_move.c`**

```c
int identity(int x) { return x; }

int main(void) {
    int v = identity(5);
    if (v != 5) return 1;
    return 42;
}
```

**`test_peephole_nop_move.cflags`**

```
-O2
```

**`test_peephole_nop_move.status`**

```
42
```

The test verifies that optimized output still produces correct results after
no-op moves are stripped.

### Unit-level check (manual, not automated)

After building, compile a minimal C file at `-O2` and inspect the NASM output
to confirm no `mov REG, REG` lines remain:

```sh
./cc99 -O2 -S -o /tmp/check.asm demos/pong.c 2>/dev/null
grep -P '^\s+mov\s+(\w+),\s+\1$' /tmp/check.asm && echo "FOUND NOP MOVES" || echo "none"
```

---

## Version

Update `src/version.c`:

```c
#define VERSION_STAGE  "01640000"
```

---

## Implementation order

1. Add `match_nop_move` and `replace_nop_move` functions to `src/peephole.c`
   (insert after the existing zero-register block, before `g_builtin_patterns`).
2. Expand `g_builtin_patterns[1]` → `g_builtin_patterns[2]` and register the
   new entry; update `*n_pats` from 1 to 2.
3. Update the file-header comment to mention Stage 164.
4. Add `test/integration/test_peephole_nop_move/` with the files above.
5. Run `./test/run_all_tests.sh` — all tests pass.
6. Run `./build.sh --mode=self-host` — C0→C1→C2 verified.
7. Bump `VERSION_STAGE` to `"01640000"` in `src/version.c`.
