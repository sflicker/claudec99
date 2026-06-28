# Stage 171 Kickoff — --help, Verbose Mode, and Mixed Input File Types

## Summary of the spec

Stage 171 addresses three related gaps in the CLI surface:

1. **--help flag** — Both `ccompiler` (C frontend) and `bin/cc99` (bash driver) lack a `--help` flag. Users expecting GCC/Clang-style discovery get no in-tool documentation.

2. **Unconditional progress noise** — `ccompiler` unconditionally prints `compiled: X -> Y` for every file it processes, which is unwanted noise in scripts and build harnesses. Conversely, `bin/cc99` silently dispatches three external tools (ccompiler, nasm, gcc) with no way to see the exact commands being run.

3. **Only `.c` inputs accepted** — `bin/cc99` rejects any non-`.c` input file. Real Makefiles routinely pass pre-assembled `.s`/`.asm` files, pre-compiled `.o` objects, and static/shared libraries (`.a`/`.so`) alongside `.c` sources. GCC and Clang accept all of these; `cc99` should too.

**Goals after this stage:**

- `ccompiler --help` prints a formatted help message to stdout and exits 0
- `cc99 --help` prints a formatted help message to stdout and exits 0
- `ccompiler -v` / `ccompiler --verbose` enables verbose mode; the `compiled: X -> Y` progress line is suppressed unless verbose is active (controlled by global `g_verbose`)
- `cc99 -v` / `cc99 --verbose` prints each tool invocation (ccompiler, nasm, gcc) to stderr before executing it, and forwards `-v` to `ccompiler`
- `cc99` accepts `.s` and `.asm` input files (NASM syntax; assembled with `nasm`, skipping the compile step)
- `cc99` accepts `.o` input files (routed directly to the linker)
- `cc99` accepts `.a` and `.so` input files (passed directly to the linker)
- All existing tests continue to pass

---

## Required tokenizer changes

None. This is a tooling/CLI stage only.

---

## Required parser changes

None. This is a tooling/CLI stage only.

---

## Required AST changes

None. This is a tooling/CLI stage only.

---

## Required code generation changes

None. This is a tooling/CLI stage only.

---

## Test plan

**Help flag functionality:**

- `ccompiler --help` exits 0 and outputs contain "Usage:"
- `cc99 --help` exits 0 and outputs contain "Usage:"

**Verbose mode in ccompiler:**

- `ccompiler /tmp/t.c -o /tmp/t.asm` (no `-v`) produces no progress output
- `ccompiler -v /tmp/t.c -o /tmp/t.asm` prints "compiled: /tmp/t.c -> /tmp/t.asm"
- `ccompiler --verbose /tmp/t.c -o /tmp/t.asm` prints "compiled: /tmp/t.c -> /tmp/t.asm"

**Verbose mode in cc99:**

- `cc99 -v -o /tmp/prog /tmp/t.c 2>&1 | grep "^cc99:"` prints at least three lines (ccompiler, nasm, gcc invocations)

**Assembly file input:**

- Produce an `.asm` file, then link it: `cc99 /tmp/t.asm -o /tmp/t_asm && /tmp/t_asm` exits 0
- Accept `.s` extension: `cc99 /tmp/t.s -o /tmp/t_s && /tmp/t_s` exits 0

**Object file input:**

- `cc99 -c /tmp/t.c -o /tmp/t.o && cc99 /tmp/t.o -o /tmp/t_obj && /tmp/t_obj` exits 0

**Mixed `.c` and `.o` inputs:**

- `cc99 /tmp/main.c /tmp/helper.o -o /tmp/mixed && /tmp/mixed` exits 0

**Static library forwarding:**

- `ar rcs /tmp/libhelper.a /tmp/helper.o && cc99 /tmp/main.c /tmp/libhelper.a -o /tmp/wlib && /tmp/wlib` exits 0

**Non-C invocation:**

- `cc99 /tmp/t.o /tmp/helper.o -o /tmp/noC && /tmp/noC` exits 0

**Existing test suite:**

- `./test/run_all_tests.sh` — all tests pass

---

## Implementation scope

**Files changing:**

- `include/util.h` — add `extern int g_verbose;`
- `src/util.c` — add `int g_verbose = 0;`
- `src/compiler.c` — add `--help` and `-v`/`--verbose` arg branches, make progress line conditional, simplify error-path usage string
- `bin/cc99` — add `--help`, `-v`/`--verbose`, `run_cmd` helper, new extension cases, rewrite compilation loop
- `src/version.c` — bump to `01710000`

**Bootstrap constraint:**

All compiler additions use only string comparisons, integer assignment, `printf`, `free`, and `return`. The `--help` output is a concatenated string literal, valid in C89/C99. The `if (g_verbose)` guard is a plain conditional. No VLAs, no `//` comments, no compound literals.

Run `./build.sh --mode=self-host` before declaring the stage complete (C0→C1→C2 must verify).
