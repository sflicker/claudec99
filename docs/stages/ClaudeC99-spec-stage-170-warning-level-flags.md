# ClaudeC99 Stage 170 — Warning Level Flags (-Wall, -Wextra)

## Background

The compiler already has a single warning-control flag, `-Werror`, that promotes
any diagnostic emitted via `compile_warning_at` into a fatal error.  The
`g_warnings_are_errors` global (declared in `include/util.h`, defined in
`src/util.c`) carries this flag from `main` into every compilation unit.

GCC and Clang expose warning *groups* via `-Wall` and `-Wextra`:

- `-Wall` — enables a curated set of warnings that almost always indicate a bug
  or a dangerous construct.  In GCC the set includes unused variables,
  uninitialized variables, implicit function declarations, missing return
  statements, mismatched format strings, and several others.
- `-Wextra` — enables additional checks that are sometimes intentional but worth
  reviewing: unused parameters, signed/unsigned comparisons, missing struct-field
  initializers, and more.  `-Wextra` is a strict superset of `-Wall` in terms of
  the behaviors it arms.

In this stage the flags are **parsed, validated, stored, and forwarded**, but
**no new warning diagnostics are emitted**.  This mirrors how Stage 142 added
`-O0`/`-O1` as pure infrastructure before Stage 143 introduced the first real
optimization rule.  The warning-check implementations are deferred to follow-up
stages (see checklist additions below).

---

## Goal

After this stage:

1. `ccompiler` accepts `-Wall` and `-Wextra` without error and records them in a
   `warn_level` integer (`0` = none, `1` = Wall, `2` = Wall + Wextra).
2. `bin/cc99` accepts and forwards both flags to `ccompiler` unchanged.
3. The usage strings for both tools are updated.
4. All existing tests continue to pass (nothing new is emitted yet).
5. The checklist is updated with per-diagnostic items for future stages.

---

## Implementation

### 1. `src/util.c` and `include/util.h` — `g_warn_level` global

Add a global warning-level counter alongside the existing `g_warnings_are_errors`:

**`include/util.h`** — after the `g_warnings_are_errors` declaration (line 35):

```c
/* Stage 170: warning group level selected by -Wall / -Wextra.
 * 0 = no warning groups; 1 = -Wall; 2 = -Wall -Wextra. */
extern int g_warn_level;
```

**`src/util.c`** — after the `g_warnings_are_errors` initializer:

```c
int g_warn_level = 0;
```

No callers use `g_warn_level` yet; future warning-check stages will gate their
`compile_warning_at` calls on `g_warn_level >= 1` (or `>= 2` for Wextra-only
diagnostics).

### 2. `src/compiler.c` — parse `-Wall` / `-Wextra`

**Local variable** — add alongside `warnings_are_errors` (around line 362):

```c
    int warn_level = 0;
```

**Argument loop** — add two new branches after the `-Werror` branch (around
line 387):

```c
        } else if (strcmp(argv[i], "-Wall") == 0) {
            if (warn_level < 1) warn_level = 1;
            g_warn_level = warn_level;
        } else if (strcmp(argv[i], "-Wextra") == 0) {
            if (warn_level < 2) warn_level = 2;
            g_warn_level = warn_level;
```

`-Wextra` implies `-Wall`, so it sets level 2 regardless of order.  Setting
`g_warn_level` immediately (rather than only at the call site) ensures that any
early diagnostic paths see the correct level.

**Usage string** (around line 464) — add `-Wall` and `-Wextra` after `-Werror`:

```c
    fprintf(stderr,
        "usage: ccompiler [--version] [--print-ast | --print-tokens] "
        "[-Werror] [-Wall] [-Wextra] [--max-errors=N] [--sysroot=<dir>] "
        "[-O0|-O1|-O2] [-g] [-DNAME[=VAL]] [-I<dir>] <source.c> [source2.c ...]\n");
```

No changes to `compile_one_file` are needed in this stage — `g_warn_level` is a
global and is visible everywhere without threading it through call chains.

### 3. `bin/cc99` — accept and forward `-Wall` / `-Wextra`

**Usage block** (around line 65, after the `-Werror` line):

```
  -Wall            Enable common warning diagnostics
  -Wextra          Enable additional warning diagnostics (implies -Wall)
```

**Argument-parsing `case` block** (around line 107, after the `-Werror)` arm):

```bash
        -Wall|-Wextra)
            compiler_flags+=("$1"); shift ;;
```

Both flags are compiler flags (not linker flags), so they accumulate in
`compiler_flags` and are forwarded to every `"$COMPILER"` invocation, exactly
like `-Werror`.

---

## Tests

### Flag acceptance — `ccompiler`

```sh
echo 'int main(void) { return 0; }' > /tmp/t.c
./build/ccompiler -Wall   /tmp/t.c   # exit 0, no unknown-flag error
./build/ccompiler -Wextra /tmp/t.c   # exit 0
./build/ccompiler -Wall -Wextra /tmp/t.c   # exit 0
./build/ccompiler -Wextra -Wall /tmp/t.c   # exit 0 (order-independent)
```

### Flag acceptance — `cc99`

```sh
bin/cc99 -Wall   -o /tmp/t /tmp/t.c && /tmp/t; echo $?  # 0
bin/cc99 -Wextra -o /tmp/t /tmp/t.c && /tmp/t; echo $?  # 0
```

### Output equivalence

Compiling with or without `-Wall` / `-Wextra` must produce identical NASM output
in this stage:

```sh
./build/ccompiler         /tmp/t.c -o /tmp/t_plain.asm
./build/ccompiler -Wall   /tmp/t.c -o /tmp/t_wall.asm
./build/ccompiler -Wextra /tmp/t.c -o /tmp/t_wextra.asm
diff /tmp/t_plain.asm /tmp/t_wall.asm    # no diff
diff /tmp/t_plain.asm /tmp/t_wextra.asm  # no diff
```

### Existing test suite

Run `./test/run_all_tests.sh` — all tests must pass.  The test runner does not
need to know about `-Wall` / `-Wextra` in this stage.

---

## Checklist additions

Because the flags are accepted but generate no diagnostics yet, the following
per-diagnostic items are added to `docs/outlines/checklist.md` under
**"Diagnostics and Error Recovery"**, indented under the warning-level item:

```markdown
- [ ] Warning level support (-Wall, -Wextra)  ← mark complete after this stage
  - [ ] -Wunused-variable: warn when a declared local variable is never read
        after its last assignment (requires liveness analysis or use-flag in
        the symbol table; -Wall)
  - [ ] -Wunused-function: warn when a `static` function is defined but never
        called within the translation unit (-Wall)
  - [ ] -Wunused-parameter: warn when a function parameter is never referenced
        in the function body (-Wextra)
  - [ ] -Wimplicit-function-declaration: warn (currently an error) when a
        function is called before it has been declared or defined; downgrade to
        warning under -Wall, error only under -Werror (-Wall)
  - [ ] -Wreturn-type: warn when a non-void function may reach the end of its
        body without a `return` statement; warn when a void function has
        `return <expr>` (-Wall)
  - [ ] -Wsign-compare: warn when a signed integer is compared to an unsigned
        integer in a relational or equality expression (-Wextra)
  - [ ] -Wmissing-field-initializers: warn when a struct or union initializer
        omits one or more named members (requires tracking which fields an
        initializer covers) (-Wextra)
  - [ ] -Wformat (future, deferred): validate printf/scanf format-string
        arguments against the format specifiers; requires type propagation
        through varargs call sites
```

---

## Bootstrap constraint

The two new lines in `src/compiler.c` and the one-liner in `src/util.c` and
`include/util.h` use only `strcmp`, integer assignment, and `extern int` —
all valid under the C89/C99 subset the self-hosted compiler handles.  No VLAs,
no `//` comments, no compound literals.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Version

Update `src/version.c`:

```c
#define VERSION_STAGE  "01700000"
```

---

## Implementation order

1. Add `extern int g_warn_level;` to `include/util.h` after the
   `g_warnings_are_errors` declaration.
2. Add `int g_warn_level = 0;` to `src/util.c` after the
   `g_warnings_are_errors` initializer.
3. Add `int warn_level = 0;` local variable in `main()` in `src/compiler.c`.
4. Add the `-Wall` and `-Wextra` branches in the argument loop in
   `src/compiler.c`.
5. Update the usage string in `src/compiler.c`.
6. Add `-Wall|-Wextra)` arm to the `case` block in `bin/cc99`.
7. Update the usage block in `bin/cc99`.
8. Run `./test/run_all_tests.sh` — all tests pass.
9. Run `./build.sh --mode=self-host` — C0→C1→C2 verified.
10. Update `docs/outlines/checklist.md`: mark "Warning level support" complete
    and add the indented per-diagnostic sub-items above.
11. Bump `VERSION_STAGE` to `"01700000"` in `src/version.c`.
