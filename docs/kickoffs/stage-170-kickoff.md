# Stage 170 Kickoff — Warning Level Flags (-Wall, -Wextra)

## Summary of the spec

Stage 170 adds support for `-Wall` and `-Wextra` warning-level flags to the compiler. The flags are parsed, validated, and stored in a new `g_warn_level` global integer (0 = none, 1 = Wall, 2 = Wall + Wextra), and forwarded through `bin/cc99`, but **no new warning diagnostics are emitted yet**. This mirrors Stage 142, which added `-O0`/`-O1` infrastructure before Stage 143 introduced actual optimization. The warning-check implementations are deferred to follow-up stages. The checklist is updated with per-diagnostic sub-items for future stages. This is a pure infrastructure stage: all existing tests pass unchanged because no new warnings are triggered.

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation changes

None.

## Implementation tasks

### 1. Add `g_warn_level` global to `include/util.h` and `src/util.c`

**`include/util.h`** — after the `g_warnings_are_errors` declaration (line 35), add:

```c
/* Stage 170: warning group level selected by -Wall / -Wextra.
 * 0 = no warning groups; 1 = -Wall; 2 = -Wall -Wextra. */
extern int g_warn_level;
```

**`src/util.c`** — after the `g_warnings_are_errors` initializer, add:

```c
int g_warn_level = 0;
```

No callers use `g_warn_level` yet; future warning-check stages will gate their `compile_warning_at` calls on `g_warn_level >= 1` (or `>= 2` for Wextra-only diagnostics).

### 2. Parse `-Wall` and `-Wextra` in `src/compiler.c`

**Local variable** (around line 362) — add alongside `warnings_are_errors`:

```c
    int warn_level = 0;
```

**Argument loop** (around line 387) — add two new branches after the `-Werror` branch:

```c
        } else if (strcmp(argv[i], "-Wall") == 0) {
            if (warn_level < 1) warn_level = 1;
            g_warn_level = warn_level;
        } else if (strcmp(argv[i], "-Wextra") == 0) {
            if (warn_level < 2) warn_level = 2;
            g_warn_level = warn_level;
```

`-Wextra` implies `-Wall`, so it sets level 2 regardless of order. Setting `g_warn_level` immediately ensures that any early diagnostic paths see the correct level.

**Usage string** (around line 464) — update to include `-Wall` and `-Wextra` after `-Werror`:

```c
    fprintf(stderr,
        "usage: ccompiler [--version] [--print-ast | --print-tokens] "
        "[-Werror] [-Wall] [-Wextra] [--max-errors=N] [--sysroot=<dir>] "
        "[-O0|-O1|-O2] [-g] [-DNAME[=VAL]] [-I<dir>] <source.c> [source2.c ...]\n");
```

### 3. Forward flags in `bin/cc99`

**Usage block** (around line 65, after the `-Werror` line), add:

```
  -Wall            Enable common warning diagnostics
  -Wextra          Enable additional warning diagnostics (implies -Wall)
```

**Argument-parsing `case` block** (around line 107, after the `-Werror)` arm), add:

```bash
        -Wall|-Wextra)
            compiler_flags+=("$1"); shift ;;
```

Both flags are compiler flags (not linker flags), so they accumulate in `compiler_flags` and are forwarded to every `"$COMPILER"` invocation, exactly like `-Werror`.

### 4. Bootstrap validation

Ensure all changes use only C89/C99 subset features the self-hosted compiler handles:
- `src/compiler.c` and `src/util.c` use only `strcmp`, integer assignment, and `extern int` — all valid
- No VLAs, no `//` comments, no compound literals

### 5. Version bump

Update `src/version.c`:

```c
#define VERSION_STAGE  "01700000"
```

### 6. Checklist update

In `docs/outlines/checklist.md`, mark **"Warning level support"** complete and expand it with per-diagnostic sub-items under **"Diagnostics and Error Recovery"**:

```markdown
- [x] Warning level support (-Wall, -Wextra)
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

## Test plan

### Flag acceptance — `ccompiler`

Test that the compiler accepts both flags without error:

```sh
echo 'int main(void) { return 0; }' > /tmp/t.c
./build/ccompiler -Wall   /tmp/t.c   # exit 0
./build/ccompiler -Wextra /tmp/t.c   # exit 0
./build/ccompiler -Wall -Wextra /tmp/t.c   # exit 0
./build/ccompiler -Wextra -Wall /tmp/t.c   # exit 0 (order-independent)
```

### Flag acceptance — `cc99`

Test that cc99 accepts both flags and produces a runnable binary:

```sh
bin/cc99 -Wall   -o /tmp/t /tmp/t.c && /tmp/t; echo $?  # 0
bin/cc99 -Wextra -o /tmp/t /tmp/t.c && /tmp/t; echo $?  # 0
```

### Output equivalence

Compiling with or without `-Wall` / `-Wextra` must produce identical NASM output in this stage (no diagnostics yet):

```sh
./build/ccompiler         /tmp/t.c -o /tmp/t_plain.asm
./build/ccompiler -Wall   /tmp/t.c -o /tmp/t_wall.asm
./build/ccompiler -Wextra /tmp/t.c -o /tmp/t_wextra.asm
diff /tmp/t_plain.asm /tmp/t_wall.asm    # no diff
diff /tmp/t_plain.asm /tmp/t_wextra.asm  # no diff
```

### Existing test suite

Run `./test/run_all_tests.sh` — all tests must pass. The test runner does not need to know about `-Wall` / `-Wextra` in this stage.

### Bootstrap validation

Run `./build.sh --mode=self-host` to confirm C0→C1→C2 self-compilation succeeds with the new code.

## Implementation order

1. Add `extern int g_warn_level;` to `include/util.h` after `g_warnings_are_errors`.
2. Add `int g_warn_level = 0;` to `src/util.c` after `g_warnings_are_errors` initializer.
3. Add `int warn_level = 0;` local variable in `main()` in `src/compiler.c`.
4. Add `-Wall` and `-Wextra` branches in the argument loop in `src/compiler.c`.
5. Update the usage string in `src/compiler.c`.
6. Add `-Wall|-Wextra)` arm to the `case` block in `bin/cc99`.
7. Update the usage block in `bin/cc99`.
8. Run `./test/run_all_tests.sh` — all tests must pass.
9. Run `./build.sh --mode=self-host` — C0→C1→C2 verified.
10. Update `docs/outlines/checklist.md`: mark "Warning level support" complete and add the indented per-diagnostic sub-items.
11. Bump `VERSION_STAGE` to `"01700000"` in `src/version.c`.

## Decisions and Ambiguities Resolved

- **Flag interaction**: `-Wextra` implies `-Wall`, so `-Wextra` alone or after `-Wall` both set level 2.
- **Order independence**: Flags can appear in any order; level is monotonic (never decreases).
- **No diagnostics yet**: This stage adds pure infrastructure; future stages will implement individual warnings.
- **Scope of warning checklist items**: Per-diagnostic items are added now to guide future implementation; they do not affect this stage.
- **Global vs. parameter**: `g_warn_level` is a global visible to all compilation units; threading through function calls is unnecessary.
