# ClaudeC99 Stage 168 — Peephole: Dead-Jump Removal

## Background

The peephole optimizer (Stage 155) uses a sliding-window engine over emitted
NASM text.  Stage 157 registered the zero-register idiom; Stage 164 added
no-op move elimination; Stage 165 added push/pop pair collapse; Stage 166
added redundant reload elimination; Stage 167 added redundant store
elimination.  All five patterns run at `-O2`.

Code generation occasionally emits a direct jump to the label that immediately
follows it:

```nasm
    jmp  .Lxx
.Lxx:
```

The jump transfers control to the very next instruction, so it is a no-op —
execution reaches `.Lxx:` regardless of whether the `jmp` is present.
Removing it shrinks code size and avoids a branch-prediction slot.

The sliding-window adjacency guarantee means the two lines are consecutive in
the window with no intervening text, so the "immediately followed by" condition
is trivially satisfied.

This stage adds the sixth built-in peephole pattern: when a `jmp LXX`
instruction is immediately followed by the label `LXX:`, delete the jump.

---

## Goal

Add a dead-jump removal pattern to `src/peephole.c`:

```
jmp  .Lxx       →   (jump deleted)
.Lxx:               .Lxx:
```

The pattern fires when `win[0]` is a `jmp` to a local label and `win[1]` is
the definition of that same label.  The jump is deleted; the label is kept
unchanged.

---

## Implementation — `src/peephole.c`

### New helper: `pp_parse_jmp_label`

Parse a line of the form `    jmp  LABEL` (leading whitespace optional, one or
more spaces between `jmp` and the target).  Copies the target name into `buf`.
Returns 1 on success, 0 otherwise.

```c
static int pp_parse_jmp_label(const char *line, char *buf, int bufsz) {
    const char *p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "jmp", 3) != 0) return 0;
    p += 3;
    if (*p != ' ' && *p != '\t') return 0;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\0') return 0;
    strncpy(buf, p, bufsz - 1);
    buf[bufsz - 1] = '\0';
    /* strip trailing whitespace */
    {
        int len = strlen(buf);
        while (len > 0 && (buf[len-1] == ' ' || buf[len-1] == '\t'
                           || buf[len-1] == '\r' || buf[len-1] == '\n'))
            buf[--len] = '\0';
    }
    return (buf[0] != '\0');
}
```

### New helper: `pp_parse_label_def`

Parse a line of the form `LABEL:` (no leading whitespace; the colon is the
terminator).  Copies the label name (without the colon) into `buf`.  Returns 1
on success, 0 otherwise.

```c
static int pp_parse_label_def(const char *line, char *buf, int bufsz) {
    const char *p = line;
    const char *colon;
    int len;
    if (*p == ' ' || *p == '\t') return 0;
    colon = strchr(p, ':');
    if (colon == NULL) return 0;
    len = (int)(colon - p);
    if (len <= 0 || len >= bufsz) return 0;
    strncpy(buf, p, len);
    buf[len] = '\0';
    return 1;
}
```

### Matcher: `match_dead_jump`

```c
static int match_dead_jump(const char **win, int n) {
    char jmp_target[64];
    char label_name[64];
    (void)n;
    if (!pp_parse_jmp_label(win[0], jmp_target, sizeof(jmp_target))) return 0;
    if (!pp_parse_label_def(win[1], label_name, sizeof(label_name))) return 0;
    return (strcmp(jmp_target, label_name) == 0);
}
```

### Replacer: `replace_dead_jump`

Delete the jump; keep the label.

```c
static void replace_dead_jump(const char **win, int n,
                              char **out, int *out_count) {
    (void)n;
    out[0]     = util_strdup(win[1]);
    *out_count = 1;
}
```

### Update `g_builtin_patterns`

Expand the static array from 5 to 6 entries and register the new pattern:

```c
static PeepholePattern g_builtin_patterns[6];

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
        g_builtin_patterns[5].window_size = 2;
        g_builtin_patterns[5].matcher     = match_dead_jump;
        g_builtin_patterns[5].replacer    = replace_dead_jump;
        g_builtin_patterns_ready          = 1;
    }
    *n_pats = 6;
    return g_builtin_patterns;
}
```

Update the file-header comment to record Stage 168:

```c
 * Stage 168: dead-jump removal -- `jmp Lxx` immediately followed by `Lxx:`
 *            -> remove the jump.
```

### Bootstrap constraint

All variable declarations (`jmp_target`, `label_name`, `p`, `colon`, `len`)
must appear at the top of their enclosing block before any executable
statements.  No VLAs; no `//` comments.  `string.h` is already available via
`util.h`.

---

## Tests

### Integration test: `test/integration/test_peephole_dead_jump/`

A portable test (no system headers required) that verifies semantic correctness
after the optimization fires at `-O2`.

**`test_peephole_dead_jump.c`**

```c
int classify(int x) {
    if (x > 0) {
        return 1;
    }
    return 0;
}

int main(void) {
    if (classify(5) != 1) return 1;
    if (classify(0) != 0) return 1;
    return 42;
}
```

The `if` statement in `classify` produces a conditional branch followed by an
unconditional `jmp` past the taken path; depending on code generation order the
`jmp` may land on the label that immediately follows it.  The optimizer must
eliminate the dead jump without changing the return values.

**`test_peephole_dead_jump.cflags`**

```
-O2
```

**`test_peephole_dead_jump.status`**

```
42
```

### Unit-level check (manual, not automated)

After building, compile the test at `-O0` vs `-O2` and confirm the dead jump is
absent from the optimized output:

```sh
./bin/cc99 -O0 -S -o /tmp/before.asm test/integration/test_peephole_dead_jump/test_peephole_dead_jump.c 2>/dev/null
./bin/cc99 -O2 -S -o /tmp/after.asm  test/integration/test_peephole_dead_jump/test_peephole_dead_jump.c 2>/dev/null
diff /tmp/before.asm /tmp/after.asm
```

---

## Version

Update `src/version.c`:

```c
#define VERSION_STAGE  "01680000"
```

---

## Implementation order

1. Add `pp_parse_jmp_label` and `pp_parse_label_def` helpers to `src/peephole.c`
   (insert after the existing `pp_parse_store_rbp` helper).
2. Add `match_dead_jump` and `replace_dead_jump` to `src/peephole.c`
   (insert after the `replace_redundant_store` block, before `g_builtin_patterns`).
3. Expand `g_builtin_patterns[5]` → `g_builtin_patterns[6]` and register the
   new entry; update `*n_pats` from 5 to 6.
4. Update the file-header comment to mention Stage 168.
5. Add `test/integration/test_peephole_dead_jump/` with the three files above.
6. Run `./test/run_all_tests.sh` — all tests pass.
7. Run `./build.sh --mode=self-host` — C0→C1→C2 verified.
8. Bump `VERSION_STAGE` to `"01680000"` in `src/version.c`.
