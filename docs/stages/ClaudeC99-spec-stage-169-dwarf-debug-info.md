# ClaudeC99 Stage 169 — Debug Information (DWARF)

## Background

The compiler emits NASM assembly (`.asm`) which is assembled with `nasm -f elf64`
into ELF64 object files.  NASM supports the DWARF debug format when assembled
with `-g -F dwarf`: it reads `%line` preprocessor directives in the assembly
source to learn which source file and line each instruction corresponds to, then
emits `.debug_line`, `.debug_abbrev`, and `.debug_info` sections in the object
file.

Each AST node carries source position information (`src_file`, `src_line`,
`src_col`) populated by the lexer since Stage 86.  `codegen_statement()` already
calls `cg_mark()` which reads these fields for error reporting.  They are never
used for output, so no DWARF annotation has been emitted so far.

This stage threads a new `-g` flag through the compiler pipeline and uses
`node->src_file` / `node->src_line` to emit `%line` directives into the
generated assembly when debug mode is active.  The NASM assembler then produces
full DWARF line-number tables, giving GDB source-level line information for any
binary compiled with `cc99 -g`.

---

## Goal

When the user passes `-g`:

1. `cc99` sets `cg.emit_debug = 1` before calling `codegen_translation_unit`.
2. `codegen_function()` emits `%line N+0 "file.c"` just before the function
   label, where N and file come from the `AST_FUNCTION_DECL` node's position.
3. `codegen_statement()` emits `%line N+0 "file.c"` at the top of each
   statement dispatch (before any assembly is written for that statement),
   deduplicating consecutive identical positions.
4. The test runner detects `-g` in the test's `.cflags` and adds `-g -F dwarf`
   to the NASM invocation, so the integration test exercises the full pipeline.

GDB can then set breakpoints by source line, step through C source, and show
correct `file:line` in `backtrace`.

---

## Implementation

### 1. `include/codegen.h` — three new fields in `CodeGen`

Add at the end of the `CodeGen` struct, before the closing `}`:

```c
    /* Stage 169: when non-zero, emit %line directives for DWARF debug info. */
    int emit_debug;
    /* Last emitted %line position — used to suppress duplicate directives. */
    const char *debug_last_file;
    int         debug_last_line;
```

### 2. `src/codegen.c` — `codegen_init`

Zero-initialize the three new fields.  `codegen_init` calls `memset` (or
explicit zeroing) on the struct before setting fields; the new members default to
zero / NULL automatically if `codegen_init` uses `memset(cg, 0, sizeof *cg)`.
Verify that is the case; if not, add:

```c
    cg->emit_debug      = 0;
    cg->debug_last_file = NULL;
    cg->debug_last_line = 0;
```

### 3. `src/codegen.c` — new helper `emit_debug_line`

Insert this static helper near the top of the file, after the existing
`cg_mark` helper:

```c
static void emit_debug_line(CodeGen *cg, const char *src_file, int src_line) {
    if (!cg->emit_debug) return;
    if (src_line <= 0 || src_file == NULL) return;
    if (src_line == cg->debug_last_line &&
        cg->debug_last_file != NULL &&
        strcmp(src_file, cg->debug_last_file) == 0) return;
    fprintf(cg->output, "%%line %d+0 \"%s\"\n", src_line, src_file);
    cg->debug_last_line = src_line;
    cg->debug_last_file = src_file;
}
```

The `%%line` in `fprintf` produces a literal `%line` in the output (one `%`
escape).  `src_file` is owned by the lexer's file pool and outlives codegen, so
storing the pointer without copying is safe.

### 4. `src/codegen.c` — `codegen_function`

Just before the line that emits the function's global/local label (the
`fprintf(cg->output, "global %s\n", ...)` or `fprintf(cg->output, "%s:\n", ...)`
call that opens the function), add:

```c
        emit_debug_line(cg, node->src_file, node->src_line);
```

This maps the function entry point to the line of its declaration.

### 5. `src/codegen.c` — `codegen_statement`

At the very top of `codegen_statement`, immediately after the `cg_mark(node);`
call (line 5713), add:

```c
    emit_debug_line(cg, node->src_file, node->src_line);
```

This annotates each statement before any assembly is emitted for it.
Deduplication in `emit_debug_line` prevents redundant directives for
sub-statements that share the same source line (e.g., the initializer
inside a `for` header).

### 6. `src/compiler.c` — parse `-g` flag

In `main`, add a local variable:

```c
    int emit_debug = 0;
```

In the argument loop, add a new branch (alongside `-O0` / `-O1` / `-O2`):

```c
        } else if (strcmp(argv[i], "-g") == 0) {
            emit_debug = 1;
```

Update the usage string to include `[-g]`.

In `compile_one_file`, add `int emit_debug` as a new parameter.  Set it on the
`CodeGen` before calling `codegen_translation_unit`:

```c
    cg.emit_debug = emit_debug;
```

Pass `emit_debug` from the `compile_one_file` call-site in `main`.

### 7. `bin/cc99` — accept `-g` and forward NASM debug flags

**Argument parser** (around line 108, alongside `-O0`/`-O1`):

```bash
        -g)
            compiler_flags+=("$1"); shift ;;
```

**Usage block** — add the line:

```
  -g               Emit DWARF debug information
```

**`emit_debug` detection** — after the argument-parsing loop, derive a flag for
NASM (alongside the existing `use_sysinclude` check):

```bash
emit_debug=0
for cflag in "${compiler_flags[@]}"; do
    if [[ "$cflag" == "-g" ]]; then emit_debug=1; break; fi
done
```

**NASM invocation** (line 205) — pass `-g -F dwarf` when debug is enabled:

```bash
    nasm_debug_flags=()
    [[ "$emit_debug" -eq 1 ]] && nasm_debug_flags=(-g -F dwarf)
    if ! nasm -f elf64 "${nasm_debug_flags[@]}" "$asm_out" -o "$obj_out"; then
```

### 8. `test/integration/run_tests.sh` — NASM debug flags

After the `compiler_flags` array is populated from `.cflags` (around line 76),
detect whether `-g` is present and build a parallel `nasm_debug_flags` array:

```bash
    nasm_debug_flags=()
    for cflag in "${compiler_flags[@]}"; do
        if [ "$cflag" = "-g" ]; then
            nasm_debug_flags=(-g -F dwarf)
            break
        fi
    done
```

Pass `"${nasm_debug_flags[@]}"` in the NASM invocation (line 130):

```bash
        if ! nasm -f elf64 "${nasm_debug_flags[@]}" \
                "$test_work/${src_name}.asm" \
                -o "$test_work/${src_name}.o" 2>/dev/null; then
```

---

## Tests

### Integration test: `test/integration/test_dwarf_debug/`

A minimal program that exercises multiple statements and functions, verifying
that the debug-annotated binary still runs correctly.

**`test_dwarf_debug.c`**

```c
static int add(int a, int b) {
    int result;
    result = a + b;
    return result;
}

static int mul(int a, int b) {
    int i;
    int acc;
    acc = 0;
    for (i = 0; i < b; i++) {
        acc = add(acc, a);
    }
    return acc;
}

int main(void) {
    int x;
    int y;
    x = mul(3, 4);
    y = add(x, 2);
    if (y != 14) return 1;
    return 42;
}
```

**`test_dwarf_debug.cflags`**

```
-g
```

**`test_dwarf_debug.status`**

```
42
```

The test verifies that:
- The compiler accepts `-g` without error.
- NASM assembles the annotated output with `-g -F dwarf` without error.
- The linked binary produces the correct exit code, confirming that `%line`
  directives do not disturb instruction ordering or correctness.

### Manual verification (not automated)

After building with `-g`:

```sh
./bin/cc99 -g test/integration/test_dwarf_debug/test_dwarf_debug.c
nasm -f elf64 -g -F dwarf test_dwarf_debug.asm -o /tmp/td.o
gcc -no-pie /tmp/td.o -o /tmp/td
readelf --debug-dump=line /tmp/td | head -40
```

Confirm that `.debug_line` is present and maps addresses to lines of
`test_dwarf_debug.c`.  Under GDB:

```sh
gdb -batch -ex "b mul" -ex "run" -ex "bt" /tmp/td
```

Confirms GDB can set a breakpoint by function name and shows the source file and
line in the backtrace.

---

## Bootstrap constraint

`emit_debug_line` uses only `fprintf`, `strcmp`, and integer comparisons —
all available in the C89/C99 subset the self-hosted compiler already handles.
All variable declarations (`src_file`, `src_line`, `cflag`, `nasm_debug_flags`)
appear at the top of their enclosing block.  No `//` comments; no VLAs.

---

## Version

Update `src/version.c`:

```c
#define VERSION_STAGE  "01690000"
```

---

## Implementation order

1. Add `emit_debug`, `debug_last_file`, `debug_last_line` to `CodeGen` in
   `include/codegen.h`.
2. Verify (or add) zero-initialization of the new fields in `codegen_init` in
   `src/codegen.c`.
3. Add `emit_debug_line` static helper to `src/codegen.c` (after `cg_mark`).
4. Call `emit_debug_line(cg, node->src_file, node->src_line)` in
   `codegen_function()` just before the function label is emitted.
5. Call `emit_debug_line(cg, node->src_file, node->src_line)` in
   `codegen_statement()` immediately after `cg_mark(node)`.
6. Add `emit_debug` local variable and `-g` branch to `main` in
   `src/compiler.c`; add the parameter to `compile_one_file`; set
   `cg.emit_debug` before `codegen_translation_unit`.
7. Update `bin/cc99`: add `-g)` branch in the argument parser, add
   `emit_debug` detection after the loop, and pass `-g -F dwarf` to NASM when
   debug is enabled.
8. Update `test/integration/run_tests.sh` to detect `-g` and pass
   `-g -F dwarf` to NASM.
9. Add `test/integration/test_dwarf_debug/` with the three files above.
10. Run `./test/run_all_tests.sh` — all tests pass.
11. Run `./build.sh --mode=self-host` — C0→C1→C2 verified.
12. Bump `VERSION_STAGE` to `"01690000"` in `src/version.c`.
