# ClaudeC99 Stage 167 — Peephole: Redundant Store Elimination

## Background

The peephole optimizer (Stage 155) uses a sliding-window engine over emitted
NASM text.  Stage 157 registered the zero-register idiom; Stage 164 added
no-op move elimination; Stage 165 added push/pop pair collapse; Stage 166
added redundant reload elimination.  All five patterns run at `-O2`.

Code generation can emit two consecutive stores to the same stack slot with no
intervening load:

```nasm
    mov [rbp - 16], rax
    mov [rbp - 16], rax
```

The first store writes a value that is immediately overwritten by the second
store before the slot is ever read.  The first store is therefore dead: it
produces no observable effect.  The sliding-window adjacency guarantee means no
instruction, label, or branch target can appear between the two matched lines,
so the "no intervening load" condition is trivially satisfied by the 2-line
window.

This stage adds the fifth built-in peephole pattern: when two consecutive
store-to-stack instructions write to the same `[rbp - N]` slot, delete the
first store.

---

## Goal

Add a redundant store elimination pattern to `src/peephole.c`:

```
mov [rbp - N], REG0         →   (first store deleted)
mov [rbp - N], REG1             mov [rbp - N], REG1
```

The pattern fires when both lines are stores to the same decimal offset N,
matched with the existing `pp_parse_store_rbp` helper from Stage 166.  The
register names may be the same or different; only the offsets need to match.
The first store is deleted; the second is kept unchanged.

---

## Implementation — `src/peephole.c`

The existing helper `pp_parse_store_rbp` (added in Stage 166) already parses
lines of the form `mov [rbp - N], REG`.  This stage reuses it directly without
modification.

### Matcher: `match_redundant_store`

```c
static int match_redundant_store(const char **win, int n) {
    char r0[32];
    char o0[16];
    char r1[32];
    char o1[16];

    (void)n;
    if (!pp_parse_store_rbp(win[0], r0, sizeof(r0), o0, sizeof(o0))) return 0;
    if (!pp_parse_store_rbp(win[1], r1, sizeof(r1), o1, sizeof(o1))) return 0;
    return (strcmp(o0, o1) == 0);
}
```

Only the offsets are compared; the register names need not match.

### Replacer: `replace_redundant_store`

Delete the first store; keep the second.

```c
static void replace_redundant_store(const char **win, int n,
                                    char **out, int *out_count) {
    (void)n;
    out[0]     = util_strdup(win[1]);
    *out_count = 1;
}
```

### Update `g_builtin_patterns`

Expand the static array from 4 to 5 entries and register the new pattern:

```c
static PeepholePattern g_builtin_patterns[5];

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
        g_builtin_patterns[4].window_size = 2;
        g_builtin_patterns[4].matcher     = match_redundant_store;
        g_builtin_patterns[4].replacer    = replace_redundant_store;
        g_builtin_patterns_ready          = 1;
    }
    *n_pats = 5;
    return g_builtin_patterns;
}
```

Update the file-header comment to record Stage 167:

```c
 * Stage 167: redundant store elimination -- two consecutive `mov [rbp-N], REG`
 *            to the same offset -> delete the first store.
```

### Bootstrap constraint

All variable declarations (`r0`, `o0`, `r1`, `o1`) must appear at the top of
their enclosing block before any executable statements.  No VLAs; no `//`
comments.  `string.h` is already available via `util.h`.

---

## Tests

### Integration test: `test/integration/test_peephole_redundant_store/`

A portable test (no system headers required) that verifies semantic correctness
after the optimization fires at `-O2`.

**`test_peephole_redundant_store.c`**

```c
int overwrite(int a, int b) {
    int x = a;
    x = b;
    return x;
}

int main(void) {
    int v = overwrite(99, 42);
    if (v != 42) return 1;
    return 42;
}
```

`overwrite` initializes a local variable to `a` and then immediately
overwrites it with `b`.  The code generator writes two consecutive stores to
the same stack slot (`[rbp - N]`) before any read; the optimizer must eliminate
the first store without changing the return value.

**`test_peephole_redundant_store.cflags`**

```
-O2
```

**`test_peephole_redundant_store.status`**

```
42
```

### Unit-level check (manual, not automated)

After building, compile the test at `-O0` vs `-O2` and confirm the first store
is absent from the optimized output:

```sh
./bin/cc99 -O0 -S -o /tmp/before.asm test/integration/test_peephole_redundant_store/test_peephole_redundant_store.c 2>/dev/null
./bin/cc99 -O2 -S -o /tmp/after.asm  test/integration/test_peephole_redundant_store/test_peephole_redundant_store.c 2>/dev/null
diff /tmp/before.asm /tmp/after.asm
```

---

## Version

Update `src/version.c`:

```c
#define VERSION_STAGE  "01670000"
```

---

## Implementation order

1. Add `match_redundant_store` and `replace_redundant_store` to `src/peephole.c`
   (insert after the `replace_redundant_reload` block, before `g_builtin_patterns`).
2. Expand `g_builtin_patterns[4]` → `g_builtin_patterns[5]` and register the
   new entry; update `*n_pats` from 4 to 5.
3. Update the file-header comment to mention Stage 167.
4. Add `test/integration/test_peephole_redundant_store/` with the three files
   above.
5. Run `./test/run_all_tests.sh` — all tests pass.
6. Run `./build.sh --mode=self-host` — C0→C1→C2 verified.
7. Bump `VERSION_STAGE` to `"01670000"` in `src/version.c`.
