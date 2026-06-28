# Stage 172 Kickoff — Build Tool Compatibility (make and cmake)

## Summary of the spec

Stage 172 addresses gaps in flag support that prevent `cc99` from being used as a drop-in compiler for **GNU make** and **CMake** projects. Build tools invoke compilers with flags that `cc99` and `ccompiler` currently reject or mishandle, causing builds to fail before any C code is compiled.

**Key problems:**

1. `-O2` is accepted by `ccompiler` but **not forwarded** by `cc99` (falls through to "unrecognized option").
2. Higher optimization levels (`-O3`, `-Os`, `-Og`, `-Ofast`) are rejected outright.
3. Language standard flags (`-std=c99`, `-std=c11`, etc.) are rejected.
4. Position-independent code flags (`-fPIC`, `-fPIE`, etc.) are rejected.
5. Hardening and code-generation flags (`-fstack-protector`, `-fvisibility`, etc.) are rejected.
6. Machine-tuning flags (`-march`, `-mtune`, `-m64`, `-m32`) are rejected.
7. System include paths (`-isystem`) are rejected.
8. Dependency-tracking flags (`-MD`, `-MF`, `-MT`, `-MQ`) are rejected.
9. Warning control flags (`-w`, `-Wno-*`) are rejected.
10. POSIX threading (`-pthread`) is rejected.

**Goals after this stage:**

- `-O2` is forwarded by `cc99` to `ccompiler`
- `-O3`, `-Os`, `-Og`, `-Ofast` are silently dropped (we max out at `-O2`)
- `-std=*` and `-ansi` are accepted as no-ops (we always target C99 semantics)
- PIC/PIE flags, hardening flags, and machine-tuning flags are silently discarded
- `-isystem <dir>` is accepted by both `ccompiler` and `cc99` (treated as `-I`)
- Dependency-tracking flags (`-MD`, `-MP`, `-MF`, `-MT`, `-MQ`) are silently discarded
- `-w` is accepted and forwarded; `-Wno-*` is silently discarded
- `-pthread` is accepted by `cc99` and appends `-lpthread` to the linker
- A new `test/build_tool/` test suite with three tests: `test_make_hello`, `test_make_cflags`, `test_cmake_hello`
- `run_all_tests.sh` includes the build_tool suite on Linux x86_64
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

**Flag acceptance — ccompiler:**

- `ccompiler -std=c99 /tmp/t.c` exits 0
- `ccompiler -std=gnu99 /tmp/t.c` exits 0
- `ccompiler -std=c11 /tmp/t.c` exits 0
- `ccompiler -ansi /tmp/t.c` exits 0
- `ccompiler -w /tmp/t.c` exits 0
- `ccompiler -isystem /usr/include /tmp/t.c` exits 0

**Flag acceptance — cc99:**

- `cc99 -O2 -o /tmp/t /tmp/t.c && /tmp/t` exits 0 (fix: `-O2` now forwarded)
- `cc99 -O3 -o /tmp/t /tmp/t.c && /tmp/t` exits 0 (silently dropped)
- `cc99 -Os -o /tmp/t /tmp/t.c && /tmp/t` exits 0 (silently dropped)
- `cc99 -std=c99 -o /tmp/t /tmp/t.c && /tmp/t` exits 0
- `cc99 -std=c11 -o /tmp/t /tmp/t.c && /tmp/t` exits 0
- `cc99 -ansi -o /tmp/t /tmp/t.c && /tmp/t` exits 0
- `cc99 -fPIC -o /tmp/t /tmp/t.c && /tmp/t` exits 0
- `cc99 -fPIE -o /tmp/t /tmp/t.c && /tmp/t` exits 0
- `cc99 -fno-pie -o /tmp/t /tmp/t.c && /tmp/t` exits 0
- `cc99 -fstack-protector -o /tmp/t /tmp/t.c && /tmp/t` exits 0
- `cc99 -march=native -m64 -o /tmp/t /tmp/t.c && /tmp/t` exits 0
- `cc99 -isystem /usr/include -o /tmp/t /tmp/t.c && /tmp/t` exits 0
- `cc99 -MD -MF /tmp/t.d -MT t.o -o /tmp/t /tmp/t.c && /tmp/t` exits 0 (no .d file produced)
- `cc99 -w -o /tmp/t /tmp/t.c && /tmp/t` exits 0
- `cc99 -Wno-unused-variable -o /tmp/t /tmp/t.c && /tmp/t` exits 0
- `cc99 --sysinclude -pthread -o /tmp/pth /tmp/pth.c && /tmp/pth` exits 0 (links pthreads)

**Build tool suite:**

- `test/build_tool/run_tests.sh` runs all three tests
- `test_make_hello` — simple hello-world compiled via `make`; outputs "hello from make"
- `test_make_cflags` — Makefile with `CFLAGS = -std=c99 -O2 -Wall`; outputs "sum: 7"
- `test_cmake_hello` — CMake project (skipped if cmake not installed); outputs "hello from cmake"

**Full test suite:**

- `./test/run_all_tests.sh` — all pre-existing tests pass; build_tool results reported separately in Linux x86_64 block

---

## Implementation scope

**Files changing:**

- `src/compiler.c` — add `-std=*`/`-ansi`, `-isystem`, `-w` branches; update `--help` and usage string
- `bin/cc99` — fix `-O2` forwarding; add `-O3`/`-Os`/`-Og`/`-Ofast`, `-std=*`/`-ansi`, `-w`, `-Wno-*`, PIC/PIE, hardening, machine-tuning, `-isystem`, dependency-tracking, and `-pthread` branches; update `--help`
- `test/build_tool/run_tests.sh` — new test runner (executable)
- `test/build_tool/test_make_hello/` — new test directory with `hello.c`, `Makefile`, `run_test.sh`, `test_make_hello.expected`
- `test/build_tool/test_make_cflags/` — new test directory with `main.c`, `Makefile`, `run_test.sh`, `test_make_cflags.expected`
- `test/build_tool/test_cmake_hello/` — new test directory with `CMakeLists.txt`, `hello.c`, `run_test.sh`, `test_cmake_hello.expected`, `test_cmake_hello.require`
- `test/run_all_tests.sh` — register build_tool suite inside Linux x86_64 block
- `src/version.c` — bump to `01720000`

**Bootstrap constraint:**

All `src/compiler.c` additions use only string comparisons (`strncmp`, `strcmp`), array indexing, `realloc`, and `fprintf` — all valid in C89/C99. The `--help` output is a concatenated string literal. No VLAs, no `//` comments, no compound literals.

Run `./build.sh --mode=self-host` before declaring the stage complete (C0→C1→C2 must verify).

---

## Files to read for context

- `docs/stages/ClaudeC99-spec-stage-172-build-tool-compatibility.md` — full implementation spec with exact code snippets
- `bin/cc99` — bash driver to understand flag handling structure
- `src/compiler.c` — C frontend to understand argument parsing patterns
- `test/build_tool/run_tests.sh` — test runner spec (in docs/stages/)
