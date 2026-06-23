# ClaudeC99 Stage 165 — Peephole: Push/Pop Pair Collapse

## Background

The peephole optimizer (Stage 155) uses a sliding-window engine over emitted
NASM text.  Stage 157 registered the zero-register idiom (`mov REG, 0` →
`xor REG32, REG32`); Stage 164 added no-op move elimination (`mov REG, REG`
→ deleted).  Both patterns use a single-line window.  The optimizer runs at
`-O2`.

Code generation occasionally emits an adjacent `push rX` / `pop rY` pair with
no branch target or label between them.  This occurs when a value is
materialized on the stack and immediately consumed: the pair is semantically
equivalent to `mov rY, rX` but costs an extra memory round-trip through the
stack.  Collapsing it to a register–register move eliminates two stack
operations and reduces text-segment size.

This stage adds the third built-in peephole pattern: collapse any consecutive
`push REG1` / `pop REG2` pair into `mov REG2, REG1`.

---

## Goal

Add a push/pop collapse pattern to `src/peephole.c`:

```
push rX          →   mov rY, rX
pop  rY
```

The pattern fires on any two adjacent lines in the emitted NASM that match
`push REG` followed immediately by `pop REG`, with no interleaving instruction
or label.  (Adjacency in the sliding window guarantees no instructions or
labels appear between the two lines.)  Indentation is stripped when matching
but the leading whitespace from the first line is preserved in the output.

Special case: if both registers are identical (`push rX` / `pop rX`), the pair
is a no-op; both lines are deleted (`out_count = 0`).

---

## Implementation — `src/peephole.c`

### Helper: `pp_extract_reg`

A small helper that extracts the register name from a `push REG` or `pop REG`
line.  The caller passes the expected mnemonic prefix (e.g. `"push "` or
`"pop "`).  Returns 1 on success, 0 on parse failure.

```c
static int pp_extract_reg(const char *line, const char *mnemonic,
                           char *dst, size_t dst_size) {
    const char *p;
    const char *reg;
    size_t mlen;
    size_t rlen;

    p    = line;
    mlen = strlen(mnemonic);
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, mnemonic, mlen) != 0) return 0;
    p += mlen;

    reg  = p;
    rlen = 0;
    while (*p != '\0') { p++; rlen++; }
    if (rlen == 0 || rlen >= dst_size) return 0;

    strncpy(dst, reg, rlen);
    dst[rlen] = '\0';
    return 1;
}
```

### Matcher: `match_push_pop`

```c
static int match_push_pop(const char **win, int n) {
    char r0[32];
    char r1[32];
    (void)n;
    if (!pp_extract_reg(win[0], "push ", r0, sizeof(r0))) return 0;
    if (!pp_extract_reg(win[1], "pop ",  r1, sizeof(r1))) return 0;
    return 1;
}
```

### Replacer: `replace_push_pop`

Preserve the leading whitespace of the `push` line.  If both registers are the
same, delete both lines; otherwise emit a single `mov rY, rX`.

```c
static void replace_push_pop(const char **win, int n,
                              char **out, int *out_count) {
    char        r0[32];
    char        r1[32];
    char        buf[80];
    const char *p;
    size_t      prefix_len;

    (void)n;

    pp_extract_reg(win[0], "push ", r0, sizeof(r0));
    pp_extract_reg(win[1], "pop ",  r1, sizeof(r1));

    /* Measure leading whitespace from the push line. */
    p = win[0];
    while (*p == ' ' || *p == '\t') p++;
    prefix_len = (size_t)(p - win[0]);
    if (prefix_len >= sizeof(buf) - 1) prefix_len = sizeof(buf) - 2;

    /* Same register: both lines are a no-op; delete them. */
    if (strcmp(r0, r1) == 0) {
        *out_count = 0;
        return;
    }

    strncpy(buf, win[0], prefix_len);
    buf[prefix_len] = '\0';
    strncat(buf, "mov ", sizeof(buf) - strlen(buf) - 1);
    strncat(buf, r1,    sizeof(buf) - strlen(buf) - 1);
    strncat(buf, ", ",  sizeof(buf) - strlen(buf) - 1);
    strncat(buf, r0,    sizeof(buf) - strlen(buf) - 1);

    out[0]     = util_strdup(buf);
    *out_count = 1;
}
```

### Update `g_builtin_patterns`

Expand the static array from 2 to 3 entries and register the new pattern:

```c
static PeepholePattern g_builtin_patterns[3];

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
        g_builtin_patterns_ready          = 1;
    }
    *n_pats = 3;
    return g_builtin_patterns;
}
```

Update the file-header comment to record Stage 165:

```c
 * Stage 165: push/pop pair collapse -- `push rX` / `pop rY` -> `mov rY, rX`.
```

### Bootstrap constraint

All variable declarations (`r0`, `r1`, `buf`, `p`, `prefix_len`, `mlen`,
`rlen`, `reg`) must appear at the top of their enclosing block before any
executable statements.  No VLAs; no `//` comments.  `string.h` is already
available via `util.h`.

---

## Tests

### Integration test: `test/integration/test_peephole_push_pop/`

A portable test (no system headers required) that verifies semantic correctness
after the optimization runs at `-O2`.

**`test_peephole_push_pop.c`**

```c
int add(int a, int b) { return a + b; }

int main(void) {
    int x = add(10, 11);
    int y = add(x, 1);
    if (y != 22) return 1;
    return 42;
}
```

**`test_peephole_push_pop.cflags`**

```
-O2
```

**`test_peephole_push_pop.status`**

```
42
```

The test verifies that the optimizer produces correct output after collapsing
any push/pop pairs found in the generated assembly.

### Unit-level check (manual, not automated)

After building, dump NASM output for a representative file and verify no naked
push/pop pairs remain:

```sh
./cc99 -O2 -S -o /tmp/check.asm demos/pong.c 2>/dev/null
grep -A1 '^\s*push ' /tmp/check.asm | grep '^\s*pop ' && echo "PAIRS FOUND" || echo "none"
```

---

## Version

Update `src/version.c`:

```c
#define VERSION_STAGE  "01650000"
```

---

## Implementation order

1. Add `pp_extract_reg`, `match_push_pop`, and `replace_push_pop` to
   `src/peephole.c` (insert after the `replace_nop_move` block, before
   `g_builtin_patterns`).
2. Expand `g_builtin_patterns[2]` → `g_builtin_patterns[3]` and register the
   new entry; update `*n_pats` from 2 to 3.
3. Update the file-header comment to mention Stage 165.
4. Add `test/integration/test_peephole_push_pop/` with the three files above.
5. Run `./test/run_all_tests.sh` — all tests pass.
6. Run `./build.sh --mode=self-host` — C0→C1→C2 verified.
7. Bump `VERSION_STAGE` to `"01650000"` in `src/version.c`.
