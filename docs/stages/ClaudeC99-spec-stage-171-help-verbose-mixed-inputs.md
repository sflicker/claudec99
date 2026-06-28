# ClaudeC99 Stage 171 — --help, Verbose Mode, and Mixed Input File Types

## Background

Three related gaps in the CLI surface are addressed together in this stage.

### 1. No `--help` flag

Both `ccompiler` and `bin/cc99` emit a terse one-liner usage only when invoked
with no input files.  Neither responds to `--help`.  Users expecting GCC/Clang-
style discovery get no in-tool documentation.

### 2. Unconditional progress noise

`ccompiler` unconditionally prints `compiled: X -> Y` for every file it
processes (line 355 of `src/compiler.c`).  Scripts and build harnesses that
invoke the compiler in a loop receive unwanted output on stdout.  Conversely,
`bin/cc99` silently dispatches three external tools (ccompiler, nasm, gcc) with
no way for the user to see the exact commands being run.

### 3. Only `.c` inputs accepted

`bin/cc99` rejects any non-`.c` input file with `cc99: unrecognized input`.
Real Makefiles and build scripts routinely pass pre-assembled `.s`/`.asm` files,
pre-compiled `.o` objects, and static/shared libraries (`.a`/`.so`) alongside
`.c` sources.  GCC and Clang accept all of these; `cc99` should too.

---

## Goal

After this stage:

1. `ccompiler --help` prints a formatted help message to stdout and exits 0.
2. `cc99 --help` prints a formatted help message to stdout and exits 0.
3. `ccompiler -v` / `ccompiler --verbose` enables verbose mode; the
   `compiled: X -> Y` progress line is suppressed unless verbose is active.
4. `cc99 -v` / `cc99 --verbose` prints each tool invocation (ccompiler, nasm,
   gcc) to stderr before executing it, and also forwards `-v` to `ccompiler`.
5. `cc99` accepts `.s` and `.asm` input files (NASM syntax; assembled with
   `nasm`, skipping the compile step).
6. `cc99` accepts `.o` input files (skips compile and assemble; routed directly
   to the linker, or treated as an already-assembled object in `-c` mode).
7. `cc99` accepts `.a` and `.so` input files in link mode (passed as positional
   arguments to the linker).
8. `cc99 foo.o bar.o -o prog` and similar all-non-C invocations work correctly
   (the "no input files" check remains; `.a`/`.so`-only is still an error).
9. All existing tests continue to pass.

---

## Implementation

### 1. `include/util.h` — `g_verbose` global

Add after the `g_warn_level` declaration:

```c
extern int g_verbose;
```

### 2. `src/util.c` — `g_verbose` initializer

Add after the `g_warn_level` initializer:

```c
int g_verbose = 0;
```

### 3. `src/compiler.c` — `--help`, `-v`/`--verbose`, conditional progress

#### Local variable in `main()`

Add alongside the other locals (around line 363):

```c
    int verbose = 0;
```

#### `--help` branch

Insert after the `--version` branch (around line 381):

```c
        } else if (strcmp(argv[i], "--help") == 0) {
            printf(
                "ccompiler -- ClaudeC99 C compiler\n"
                "\n"
                "Usage: ccompiler [options] <source.c> [source2.c ...]\n"
                "\n"
                "Options:\n"
                "  --version          Print version and exit\n"
                "  --help             Print this help and exit\n"
                "  --print-ast        Print AST to stdout and exit\n"
                "  --print-tokens     Print token stream to stdout and exit\n"
                "  -v, --verbose      Show compilation progress (compiled: X -> Y)\n"
                "  -Werror            Treat warnings as errors\n"
                "  -Wall              Enable common warning diagnostics\n"
                "  -Wextra            Enable additional diagnostics (implies -Wall)\n"
                "  --max-errors=N     Stop after N errors (0 = unlimited)\n"
                "  --sysroot=<dir>    Virtual filesystem root for absolute -I paths\n"
                "  -O0                Disable optimization (default)\n"
                "  -O1                Enable AST-level optimization\n"
                "  -O2                Enable peephole optimization\n"
                "  -g                 Emit DWARF debug information\n"
                "  -DNAME[=VAL]       Define a preprocessor macro\n"
                "  -I<dir>            Add directory to include search path\n"
                "  -I <dir>           (two-argument form)\n"
            );
            free(defines); free(include_dirs); free(source_files);
            return 0;
```

#### `-v`/`--verbose` branch

Insert after the `-Wextra` branch (around line 395):

```c
        } else if (strcmp(argv[i], "-v") == 0 ||
                   strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
            g_verbose = 1;
```

`verbose` is only used by the local `if (g_verbose)` guard below; the
assignment to `g_verbose` makes it visible to helper modules that include
`util.h` without needing to thread the flag through every call chain.

#### Conditional progress message

Change line 355:

```c
    printf("compiled: %s -> %s\n", source_file, output_path);
```

to:

```c
    if (g_verbose) printf("compiled: %s -> %s\n", source_file, output_path);
```

#### Simplified error-path usage string (line 471)

Replace the long one-liner with a short pointer:

```c
    fprintf(stderr, "ccompiler: no input files. Run 'ccompiler --help' for usage.\n");
```

---

### 4. `bin/cc99` — `--help`, `-v`/`--verbose`, mixed input file types

#### New state variable

Add alongside `use_sysinclude=0`:

```bash
verbose=0
```

#### `run_cmd` helper function

Add immediately after the `usage()` function body:

```bash
# Print the command to stderr when verbose, then execute it.
run_cmd() {
    [[ "$verbose" -eq 1 ]] && echo "cc99:" "$@" >&2
    "$@"
}
```

All subsequent `"$COMPILER"`, `nasm`, and `gcc` invocations are replaced with
`run_cmd "$COMPILER"`, `run_cmd nasm`, and `run_cmd gcc` respectively.

#### `--help` case (in the argument `case` block)

Insert before the `--version)` arm:

```bash
        --help)
            cat <<'EOF'
cc99 -- ClaudeC99 compiler frontend

Usage: cc99 [options] <input> [input2 ...]

Input file types:
  file.c            C source file     (compile -> assemble -> link)
  file.s, file.asm  Assembly file     (assemble -> link; NASM syntax)
  file.o            Object file       (link directly)
  file.a            Static library    (link directly)
  file.so           Shared library    (link directly)

Options:
  --version         Print compiler version and exit
  --help            Print this help and exit
  -v, --verbose     Print each tool invocation before running it
  -o <file>         Output file name (default: a.out)
  -c                Compile/assemble only; produce .o files
  -S                Compile only; produce .asm files (C inputs only)
  --print-ast       Print AST to stdout and exit (C inputs only)
  --print-tokens    Print token stream to stdout and exit (C inputs only)
  -D NAME[=VAL]     Define a preprocessor macro
  -I <dir>          Add directory to include search path
  -I<dir>           (same, one-argument form)
  --sysroot=<dir>   Set virtual filesystem root for absolute -I paths
  --sysinclude      Use system include paths (Linux x86_64 only)
  -O0               Disable optimization (default)
  -O1               Enable AST-level optimization
  -g                Emit DWARF debug information
  -Wall             Enable common warning diagnostics
  -Wextra           Enable additional diagnostics (implies -Wall)
  -Werror           Treat warnings as errors
  --max-errors=N    Stop after N errors (0 = unlimited)
  -l <lib>          Link with library
  -l<lib>           (same, one-argument form)
  -L <dir>          Add library search path
  -L<dir>           (same, one-argument form)
EOF
            exit 0 ;;
```

#### `-v`/`--verbose` case

Insert after the `-Werror|-Wall|-Wextra)` arm:

```bash
        -v|--verbose)
            verbose=1; compiler_flags+=("-v"); shift ;;
```

Adding `-v` to `compiler_flags` immediately ensures it is forwarded to every
`run_cmd "$COMPILER"` call without further bookkeeping.

#### Extended input file classification

Replace the existing `*.c)` and `*)` arms at the bottom of the `case` block:

```bash
        *.c)
            input_files+=("$1"); shift ;;
        *.s|*.asm)
            input_files+=("$1"); shift ;;
        *.o)
            input_files+=("$1"); shift ;;
        *.a|*.so)
            link_flags+=("$1"); shift ;;
        --)
            shift; input_files+=("$@"); break ;;
        -*)
            echo "cc99: unrecognized option: $1" >&2; exit 1 ;;
        *)
            echo "cc99: unrecognized input: $1" >&2; exit 1 ;;
```

`.a` and `.so` files are added directly to `link_flags` at parse time; they
never enter the compilation loop.  This means `cc99 libfoo.a -o prog` still
fails with "no input files" (correct — there is nothing to link from), while
`cc99 foo.o libfoo.a -o prog` works because `foo.o` is in `input_files`.

#### Extended compilation loop

The compilation loop currently assumes every `input_files` entry is a `.c`
file.  Replace the loop body with a dispatch on extension:

```bash
for src in "${input_files[@]}"; do
    abs_src="$(realpath "$src" 2>/dev/null)"
    if [[ -z "$abs_src" || ! -f "$abs_src" ]]; then
        echo "cc99: $src: no such file" >&2
        exit 1
    fi

    # Determine pipeline entry point from the file's extension.
    case "$src" in
        *.c)   needs_compile=1 ;;
        *.s|*.asm) needs_compile=0 ;;
        *.o)
            # Already an object file — add directly to link list and skip.
            if [[ "$mode" == "asm-only" ]]; then
                echo "cc99: $src: skipping object file in -S mode" >&2
            else
                obj_files+=("$abs_src")
            fi
            continue ;;
    esac

    # Derive base name without the source extension for intermediate files.
    base="$(basename "$src")"
    base="${base%.*}"

    asm_out="$tmpdir/${base}.asm"
    obj_out="$tmpdir/${base}.o"

    # ── compile: .c -> .asm (C sources only) ─────────────────────────────────
    if [[ "$needs_compile" -eq 1 ]]; then
        if [[ "$use_sysinclude" -eq 1 ]]; then
            active_iflags=("${SYSINCLUDE_IFLAGS[@]}")
        else
            active_iflags=("${DEFAULT_IFLAGS[@]}")
        fi
        if ! (cd "$tmpdir" && run_cmd "$COMPILER" "${compiler_flags[@]}" \
                "${active_iflags[@]}" "$abs_src"); then
            exit 1
        fi
    else
        # .s / .asm: copy into tmpdir under the expected name so the assemble
        # step below can reference $asm_out uniformly.
        cp "$abs_src" "$asm_out"
    fi

    if [[ "$mode" == "print" ]]; then
        continue
    fi

    if [[ "$mode" == "asm-only" ]]; then
        dest="${base}.asm"
        [[ ${#input_files[@]} -eq 1 && "$output" != "a.out" ]] && dest="$output"
        cp "$asm_out" "$dest"
        continue
    fi

    # ── assemble: .asm -> .o ──────────────────────────────────────────────────
    nasm_debug_flags=()
    [[ "$emit_debug" -eq 1 ]] && nasm_debug_flags=(-g -F dwarf)
    if ! run_cmd nasm -f elf64 "${nasm_debug_flags[@]}" "$asm_out" -o "$obj_out"; then
        echo "cc99: assembly failed for $src" >&2
        exit 1
    fi

    if [[ "$mode" == "compile-only" ]]; then
        dest="${base}.o"
        [[ ${#input_files[@]} -eq 1 && "$output" != "a.out" ]] && dest="$output"
        cp "$obj_out" "$dest"
        continue
    fi

    obj_files+=("$obj_out")
done
```

#### Link step

Replace the bare `gcc` call with `run_cmd`:

```bash
if [[ "$mode" == "link" ]]; then
    if ! run_cmd gcc -no-pie "${obj_files[@]}" "${link_flags[@]}" -o "$output"; then
        echo "cc99: link failed" >&2
        exit 1
    fi
fi
```

---

## Tests

### `--help` exits 0

```sh
./build/ccompiler --help; echo "exit: $?"          # exit: 0
bin/cc99 --help; echo "exit: $?"                   # exit: 0
```

Both outputs must include the word "Usage:" and list at least `-v`.

### Verbose output from `ccompiler`

```sh
echo 'int main(void){return 0;}' > /tmp/t.c
./build/ccompiler /tmp/t.c -o /tmp/t.asm           # no output
./build/ccompiler -v /tmp/t.c -o /tmp/t.asm        # prints "compiled: ..."
./build/ccompiler --verbose /tmp/t.c -o /tmp/t.asm # same
```

### Verbose output from `cc99`

```sh
bin/cc99 -v -o /tmp/t /tmp/t.c 2>&1 | grep "^cc99:"
# Must print at least three lines: one for ccompiler, one for nasm, one for gcc
```

### Assembly file input

```sh
# Produce an .asm file, then link it directly.
./build/ccompiler -v /tmp/t.c -o /tmp/t.asm
bin/cc99 /tmp/t.asm -o /tmp/t_asm && /tmp/t_asm; echo $?   # 0

# Also accept .s extension.
cp /tmp/t.asm /tmp/t.s
bin/cc99 /tmp/t.s -o /tmp/t_s && /tmp/t_s; echo $?         # 0
```

### Object file input

```sh
bin/cc99 -c /tmp/t.c -o /tmp/t.o
bin/cc99 /tmp/t.o -o /tmp/t_obj && /tmp/t_obj; echo $?     # 0
```

### Mixed `.c` + `.o`

```sh
cat > /tmp/helper.c <<'C'
int add(int a, int b) { return a + b; }
C
cat > /tmp/main.c <<'C'
int add(int,int);
int main(void) { return add(1,-1); }
C
bin/cc99 -c /tmp/helper.c -o /tmp/helper.o
bin/cc99 /tmp/main.c /tmp/helper.o -o /tmp/mixed && /tmp/mixed; echo $?  # 0
```

### `.a` / `.so` forwarded to linker

```sh
# Build a trivial static library.
bin/cc99 -c /tmp/helper.c -o /tmp/helper.o
ar rcs /tmp/libhelper.a /tmp/helper.o

bin/cc99 /tmp/main.c /tmp/libhelper.a -o /tmp/wlib && /tmp/wlib; echo $?  # 0
```

### No-C invocation

```sh
bin/cc99 /tmp/t.o /tmp/helper.o -o /tmp/noC && /tmp/noC; echo $?  # 0
```

### Existing test suite

```sh
./test/run_all_tests.sh
```

All tests must pass.  The test runner does not pass any `.s`/`.o`/`.a` inputs,
so the extended classification is not exercised by existing integration tests
and requires manual spot-checks above.

---

## Bootstrap constraint

All `src/compiler.c` additions use only string comparisons, integer assignment,
`printf`, `free`, and `return`.  The `--help` output is a concatenated string
literal — valid in C89/C99 and supported by the self-hosted compiler.

The `if (g_verbose)` guard is a plain conditional and is already-parsed
syntax.  No VLAs, no `//` comments, no compound literals.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Version

Update `src/version.c`:

```c
#define VERSION_STAGE  "01710000"
```

---

## Implementation order

1. Add `extern int g_verbose;` to `include/util.h` after `g_warn_level`.
2. Add `int g_verbose = 0;` to `src/util.c` after `g_warn_level`.
3. Add `int verbose = 0;` local variable to `main()` in `src/compiler.c`.
4. Insert the `--help` branch in the argument loop in `src/compiler.c`.
5. Insert the `-v`/`--verbose` branch in the argument loop in `src/compiler.c`.
6. Wrap the `printf("compiled: ...")` line in `if (g_verbose)`.
7. Simplify the error-path usage string in `src/compiler.c`.
8. Add the `verbose=0` state variable to `bin/cc99`.
9. Add the `run_cmd` helper function to `bin/cc99`.
10. Add the `--help` case to the argument `case` block in `bin/cc99`.
11. Add the `-v|--verbose` case to the argument `case` block in `bin/cc99`.
12. Extend the input file classification arms (`.s`/`.asm`/`.o`/`.a`/`.so`)
    in `bin/cc99`.
13. Replace the compilation loop body in `bin/cc99` with the extension-
    dispatching version above.
14. Replace the bare `gcc` link call with `run_cmd gcc`.
15. Run `./test/run_all_tests.sh` — all tests pass.
16. Run the manual spot-checks from the Tests section above.
17. Run `./build.sh --mode=self-host` — C0→C1→C2 verified.
18. Bump `VERSION_STAGE` to `"01710000"` in `src/version.c`.
