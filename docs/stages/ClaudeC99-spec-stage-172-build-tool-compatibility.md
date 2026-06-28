# ClaudeC99 Stage 172 — Build Tool Compatibility (make and cmake)

## Background

`cc99` can already be used as a drop-in C compiler for simple invocations, but
build tools such as **GNU make** and **CMake** invoke compilers with a richer
set of flags that `cc99` and `ccompiler` currently reject or silently ignore in
the wrong place.  This creates two problems:

1. **Unknown flags cause hard failures.**  When a Makefile sets
   `CFLAGS = -std=c99 -O2 -fPIC`, invoking `cc99` with those flags exits
   non-zero with "unrecognized option".  The build fails before any C code is
   touched.

2. **No automated test exercises build-tool-driven compilation.**  All existing
   integration tests call `ccompiler` or `cc99` directly with a curated flag
   set.  There is no test that runs `make` or `cmake` and verifies the resulting
   binary.

### Specific gaps

| Flag(s) | Tool | Current behaviour |
|---------|------|-------------------|
| `-O2` | `cc99` | Accepted by `ccompiler` but **not forwarded** by `cc99` (falls through to "unrecognized option") |
| `-O3 -Os -Og -Ofast` | `cc99` | Rejected |
| `-std=c99` (and variants) | both | Rejected |
| `-fPIC -fPIE -fpic -fpie -fno-pie …` | `cc99` | Rejected |
| `-fstack-protector …` | `cc99` | Rejected |
| `-fvisibility=…` | `cc99` | Rejected |
| `-pipe` | `cc99` | Rejected |
| `-march=… -mtune=… -m64` | `cc99` | Rejected |
| `-isystem <dir>` | both | Rejected |
| `-MD -MP -MF <f> -MT <t> -MQ <t>` | `cc99` | Rejected |
| `-w` | both | Rejected |
| `-Wno-…` | `cc99` | Rejected |
| `-pthread` | `cc99` | Rejected |

---

## Goal

After this stage:

1. `cc99` forwards `-O2` to `ccompiler` (fixing the existing gap alongside
   `-O0` and `-O1`).
2. `cc99` accepts `-O3`, `-Os`, `-Og`, `-Ofast` and silently drops them (our
   best optimisation level is `-O2`; no forward is made).
3. `cc99` and `ccompiler` accept `-std=c99`, `-std=gnu99`, `-std=c11`,
   `-std=gnu11`, `-std=c17`, `-std=gnu17`, `-std=c2x`, and `-ansi` as
   no-ops (we always target C99 semantics).
4. `cc99` accepts and discards all common PIC/PIE flags:
   `-fPIC -fPIE -fpic -fpie -fno-pie -fno-PIC -fno-pic -fno-PIE`.
5. `cc99` accepts and discards common hardening and code-generation flags:
   `-fstack-protector`, `-fstack-protector-all`, `-fstack-protector-strong`,
   `-fno-stack-protector`, `-fvisibility=…`, `-fomit-frame-pointer`,
   `-fno-omit-frame-pointer`, `-fstrict-aliasing`, `-fno-strict-aliasing`,
   `-ffunction-sections`, `-fdata-sections`, `-pipe`.
6. `cc99` accepts and discards machine-tuning flags:
   `-march=…`, `-mtune=…`, `-m64`, `-m32`.
7. `cc99` accepts `-isystem <dir>` and forwards it to `ccompiler` as `-I<dir>`;
   `ccompiler` also accepts `-isystem <dir>` and treats it as `-I<dir>`.
8. `cc99` accepts and discards dependency-tracking flags:
   `-MD`, `-MP`, `-MF <file>`, `-MT <target>`, `-MQ <target>`.
9. `cc99` accepts `-w` and forwards it to `ccompiler`; `ccompiler` accepts `-w`
   and records it (no warnings are emitted yet; the flag will take effect when
   warning diagnostics are added in future stages).
10. `cc99` accepts and discards `-Wno-<anything>` flags (individual warning
    suppressions; no diagnostics are emitted yet).
11. `cc99` accepts `-pthread` and appends `-lpthread` to `link_flags`.
12. A new **`test/build_tool/`** test suite is added with three tests:
    - `test_make_hello` — simple hello-world compiled via a Makefile.
    - `test_make_cflags` — Makefile that sets `CFLAGS = -std=c99 -O2 -Wall`.
    - `test_cmake_hello` — CMake-based project (skipped if `cmake` is absent).
13. `run_all_tests.sh` runs the `build_tool` suite on Linux x86_64 and reports
    its results in the aggregate.
14. All existing tests continue to pass.

---

## Implementation

### 1. `src/compiler.c` — new flag branches

#### `-std=…` and `-ansi`

After the `-O2` branch (around line 458):

```c
        } else if (strncmp(argv[i], "-std=", 5) == 0 ||
                   strcmp(argv[i], "-ansi") == 0) {
            /* accepted, ignored — we always target C99 semantics */
        } else if (strcmp(argv[i], "-isystem") == 0) {
            const char *ipath;
            if (i + 1 >= argc) {
                fprintf(stderr, "error: -isystem requires an argument\n");
                free(defines); free(include_dirs); free(source_files);
                return 1;
            }
            ipath = argv[++i];
            if (n_include_dirs == include_dirs_cap) {
                include_dirs_cap = include_dirs_cap * 2 + 8;
                include_dirs = realloc(include_dirs,
                                       (size_t)include_dirs_cap * sizeof(const char *));
                if (!include_dirs) {
                    fprintf(stderr, "error: out of memory\n");
                    free(defines); free(source_files);
                    return 1;
                }
            }
            include_dirs[n_include_dirs++] = ipath;
        } else if (strcmp(argv[i], "-w") == 0) {
            /* suppress all warnings — no-op until warning diagnostics land */
```

Update the `--help` output to mention `-std=`, `-isystem`, and `-w` and add
them to the usage string.

### 2. `bin/cc99` — extended flag handling

All changes are in the `while [[ $# -gt 0 ]]; do case "$1" in` block.

#### Fix `-O2` forwarding and add `-O3`/`-Os`/`-Og`/`-Ofast`

Replace the existing:

```bash
        -O0|-O1|-g)
            compiler_flags+=("$1"); shift ;;
```

with:

```bash
        -O0|-O1|-O2|-g)
            compiler_flags+=("$1"); shift ;;
        -O3|-Os|-Og|-Ofast)
            # Higher/alternative optimisation levels — silently drop; our max
            # is -O2.
            shift ;;
```

#### `-std=…` and `-ansi`

After the `-Werror|-Wall|-Wextra` arm:

```bash
        -std=*|-ansi)
            compiler_flags+=("$1"); shift ;;
```

These are forwarded so `ccompiler` can record them (and future stages can act
on them).

#### `-w` and `-Wno-…`

```bash
        -w)
            compiler_flags+=("$1"); shift ;;
        -Wno-*)
            # Individual warning suppressions — silently discard.
            shift ;;
```

#### PIC/PIE flags

```bash
        -fPIC|-fPIE|-fpic|-fpie|-fno-pie|-fno-PIC|-fno-pic|-fno-PIE)
            # Our output is position-independent by construction; discard.
            shift ;;
```

#### Hardening and code-generation `-f…` flags

```bash
        -fstack-protector|-fstack-protector-all|-fstack-protector-strong|-fno-stack-protector)
            shift ;;
        -fvisibility=*|-fomit-frame-pointer|-fno-omit-frame-pointer)
            shift ;;
        -fstrict-aliasing|-fno-strict-aliasing|-ffunction-sections|-fdata-sections)
            shift ;;
        -pipe)
            shift ;;
```

#### Machine-tuning flags

```bash
        -march=*|-mtune=*|-m64|-m32)
            shift ;;
```

#### `-isystem`

```bash
        -isystem)
            [[ $# -lt 2 ]] && { echo "cc99: -isystem requires an argument" >&2; exit 1; }
            dir="$2"
            [[ "$dir" != /* ]] && dir="$(pwd)/$dir"
            compiler_flags+=("-isystem" "$dir"); shift 2 ;;
```

#### Dependency-tracking flags (make/cmake use `-MD`, `-MF`, `-MT`, `-MQ`)

```bash
        -MD|-MP)
            # Dependency generation — silently discard (no .d file emitted).
            shift ;;
        -MF|-MT|-MQ)
            # Two-argument dependency flags — discard flag and its argument.
            [[ $# -lt 2 ]] && { echo "cc99: $1 requires an argument" >&2; exit 1; }
            shift 2 ;;
```

#### `-pthread`

```bash
        -pthread)
            link_flags+=("-lpthread"); shift ;;
```

#### Update the `--help` output in `bin/cc99`

Add a "Build tool compatibility" section to the help text (after the `-Werror`
line):

```
  -std=<std>        Language standard (accepted; always compiled as C99)
  -w                Suppress all warnings
  -Wno-<name>       Suppress specific warning (accepted; no-op)
  -fPIC, -fPIE      Position-independent flags (accepted; no-op)
  -MD, -MF, -MT …   Dependency tracking (accepted; no .d file written)
  -march=…, -m64    Machine-tuning flags (accepted; no-op)
  -isystem <dir>    System include path (forwarded as -I)
  -pthread          Link with pthreads
```

---

### 3. New test suite `test/build_tool/`

#### `test/build_tool/run_tests.sh`

```bash
#!/bin/bash
#
# Build-tool compatibility test runner.
# Each subdirectory has a run_test.sh that drives make or cmake.
# An optional <name>.require file gates the test on a shell expression.
#
# Results line format (consumed by run_all_tests.sh):
#   Results: P passed, F failed, S skipped, T total
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
CC99="$PROJECT_DIR/bin/cc99"
WORK_DIR="$PROJECT_DIR/build/test_tmp_build_tool"
TIMEOUT=${CLAUDEC99_TEST_TIMEOUT:-60}

mkdir -p "$WORK_DIR"

pass=0
fail=0
skip=0
total=0

for test_dir in "$SCRIPT_DIR"/*/; do
    [ -d "$test_dir" ] || continue
    name=$(basename "$test_dir")
    total=$((total + 1))

    require_file="$test_dir/${name}.require"
    if [ -f "$require_file" ]; then
        req_cmd="$(cat "$require_file")"
        if ! eval "$req_cmd" >/dev/null 2>&1; then
            echo "SKIP  $name  (requires: $req_cmd)"
            skip=$((skip + 1))
            continue
        fi
    fi

    runner="$test_dir/run_test.sh"
    if [ ! -x "$runner" ]; then
        echo "SKIP  $name  (no executable run_test.sh)"
        skip=$((skip + 1))
        continue
    fi

    test_work="$WORK_DIR/$name"
    rm -rf "$test_work"
    cp -a "$test_dir" "$test_work"

    expected_file="$test_dir/${name}.expected"

    stdout_file="$test_work/${name}.stdout"
    stderr_file="$test_work/${name}.stderr"

    set +e
    (cd "$test_work" && timeout "$TIMEOUT" bash run_test.sh "$CC99" \
        >"$stdout_file" 2>"$stderr_file")
    rc=$?
    set -e

    if [ "$rc" -ne 0 ]; then
        echo "FAIL  $name  (exit $rc)"
        if [ -s "$stderr_file" ]; then
            head -5 "$stderr_file" | sed 's/^/      /' >&2
        fi
        fail=$((fail + 1))
        continue
    fi

    if [ -f "$expected_file" ]; then
        if ! diff -q "$expected_file" "$stdout_file" >/dev/null; then
            echo "FAIL  $name  (output mismatch)"
            diff "$expected_file" "$stdout_file" | head -10 >&2 || true
            fail=$((fail + 1))
            continue
        fi
    fi

    echo "PASS  $name"
    pass=$((pass + 1))
done

echo ""
echo "Results: $pass passed, $fail failed, $skip skipped, $total total"
```

---

#### `test/build_tool/test_make_hello/`

**`hello.c`**

```c
#include <stdio.h>

int main(void)
{
    printf("hello from make\n");
    return 0;
}
```

**`Makefile`**

```makefile
CC99 ?= cc99

.PHONY: all

all: hello

hello: hello.o
	$(CC99) -o hello hello.o

hello.o: hello.c
	$(CC99) -c -o hello.o hello.c
```

**`run_test.sh`**

```bash
#!/bin/bash
set -e
CC99="${1:-cc99}"
make -f Makefile CC99="$CC99" --no-print-directory
./hello
```

**`test_make_hello.expected`**

```
hello from make
```

---

#### `test/build_tool/test_make_cflags/`

This test exercises the CFLAGS path: `-std=c99 -O2 -Wall` must all be accepted
without error.

**`main.c`**

```c
#include <stdio.h>

int add(int a, int b) { return a + b; }

int main(void)
{
    printf("sum: %d\n", add(3, 4));
    return 0;
}
```

**`Makefile`**

```makefile
CC99  ?= cc99
CFLAGS = -std=c99 -O2 -Wall

.PHONY: all

all: main

main: main.o
	$(CC99) -o main main.o

main.o: main.c
	$(CC99) $(CFLAGS) -c -o main.o main.c
```

**`run_test.sh`**

```bash
#!/bin/bash
set -e
CC99="${1:-cc99}"
make -f Makefile CC99="$CC99" --no-print-directory
./main
```

**`test_make_cflags.expected`**

```
sum: 7
```

---

#### `test/build_tool/test_cmake_hello/`

**`test_cmake_hello.require`**

```
command -v cmake
```

**`CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.10)
project(hello C)
add_executable(hello hello.c)
```

**`hello.c`**

```c
#include <stdio.h>

int main(void)
{
    printf("hello from cmake\n");
    return 0;
}
```

**`run_test.sh`**

```bash
#!/bin/bash
set -e
CC99="${1:-cc99}"
mkdir -p build
cmake -B build -S . \
    -DCMAKE_C_COMPILER="$CC99" \
    -DCMAKE_C_FLAGS="" \
    --no-warn-unused-cli \
    -Wno-dev \
    >/dev/null
cmake --build build --verbose 2>/dev/null
./build/hello
```

**`test_cmake_hello.expected`**

```
hello from cmake
```

---

### 4. Register `build_tool` in `run_all_tests.sh`

Add the build_tool suite inside the existing Linux x86_64 block, after the
`integration_sysinclude` call:

```bash
    build_tool_runner="$SCRIPT_DIR/build_tool/run_tests.sh"
    if [ -x "$build_tool_runner" ]; then
        echo ""
        echo "===================================================="
        echo "Running suite: build tool compatibility (Linux x86_64)"
        echo "===================================================="
        bt_output=$("$build_tool_runner" 2>&1)
        bt_rc=$?
        echo "$bt_output"
        bt_summary=$(echo "$bt_output" | tail -n 1)
        if [[ "$bt_summary" =~ Results:\ ([0-9]+)\ passed,\ ([0-9]+)\ failed,\ ([0-9]+)\ skipped,\ ([0-9]+)\ total ]]; then
            btp="${BASH_REMATCH[1]}"
            btf="${BASH_REMATCH[2]}"
            btsk="${BASH_REMATCH[3]}"
            btt="${BASH_REMATCH[4]}"
        else
            btp=0; btf=0; btsk=0; btt=0
            echo "WARN  could not parse summary line for build tool suite"
        fi
        echo ""
        echo "===================================================="
        echo "Build tool: $btp passed, $btf failed, $btsk skipped, $btt total"
        echo "===================================================="
        if [ "$bt_rc" -ne 0 ]; then
            overall_rc=1
        fi
    else
        echo "SKIP  build tool suite (runner not found at $build_tool_runner)"
    fi
```

---

## Tests

### Unit-level flag acceptance — `ccompiler`

```sh
echo 'int main(void){return 0;}' > /tmp/t.c
./build/ccompiler -std=c99   /tmp/t.c     # exit 0
./build/ccompiler -std=gnu99 /tmp/t.c     # exit 0
./build/ccompiler -std=c11   /tmp/t.c     # exit 0
./build/ccompiler -ansi      /tmp/t.c     # exit 0
./build/ccompiler -w         /tmp/t.c     # exit 0
./build/ccompiler -isystem /usr/include /tmp/t.c   # exit 0
```

### Unit-level flag acceptance — `cc99`

```sh
# -O2 now forwarded
bin/cc99 -O2 -o /tmp/t /tmp/t.c && /tmp/t; echo $?   # 0

# Higher optimisation levels silently dropped
bin/cc99 -O3 -o /tmp/t /tmp/t.c && /tmp/t; echo $?   # 0
bin/cc99 -Os -o /tmp/t /tmp/t.c && /tmp/t; echo $?   # 0

# Standard flags forwarded
bin/cc99 -std=c99  -o /tmp/t /tmp/t.c && /tmp/t; echo $?   # 0
bin/cc99 -std=c11  -o /tmp/t /tmp/t.c && /tmp/t; echo $?   # 0
bin/cc99 -ansi     -o /tmp/t /tmp/t.c && /tmp/t; echo $?   # 0

# PIC/PIE flags silently discarded
bin/cc99 -fPIC -o /tmp/t /tmp/t.c && /tmp/t; echo $?   # 0
bin/cc99 -fPIE -o /tmp/t /tmp/t.c && /tmp/t; echo $?   # 0
bin/cc99 -fno-pie -o /tmp/t /tmp/t.c && /tmp/t; echo $?  # 0

# Hardening flags silently discarded
bin/cc99 -fstack-protector -o /tmp/t /tmp/t.c && /tmp/t; echo $?  # 0

# Machine flags silently discarded
bin/cc99 -march=native -m64 -o /tmp/t /tmp/t.c && /tmp/t; echo $?  # 0

# isystem forwarded as -I
bin/cc99 -isystem /usr/include -o /tmp/t /tmp/t.c && /tmp/t; echo $?  # 0

# Dependency flags silently discarded
bin/cc99 -MD -MF /tmp/t.d -MT t.o -o /tmp/t /tmp/t.c && /tmp/t; echo $?  # 0
[ ! -f /tmp/t.d ] && echo "no .d file produced (correct)"

# -w forwarded
bin/cc99 -w -o /tmp/t /tmp/t.c && /tmp/t; echo $?   # 0

# -Wno-* discarded
bin/cc99 -Wno-unused-variable -o /tmp/t /tmp/t.c && /tmp/t; echo $?  # 0

# -pthread links pthreads
cat > /tmp/pth.c << 'C'
#include <pthread.h>
int main(void) { return 0; }
C
bin/cc99 --sysinclude -pthread -o /tmp/pth /tmp/pth.c && /tmp/pth; echo $?  # 0
```

### Build tool suite

```sh
test/build_tool/run_tests.sh
# Expected: all three tests pass (cmake skipped if cmake not installed)
```

### Full test suite

```sh
./test/run_all_tests.sh
```

All pre-existing tests must pass.  The new `build_tool` results are
reported separately inside the Linux x86_64 block.

---

## Notes on cmake compatibility

cmake performs a compiler-detection step before any user project code is built.
It compiles `CMakeCCompilerId.c` (a cmake-generated probe file) with flags
including `-fPIE -MD -MF … -MT …`.  The critical behaviours:

- **`-fPIE`** must be accepted — handled by this stage.
- **`-MD -MF <f> -MT <t>`** must be accepted without writing a `.d` file —
  handled by this stage (flags are silently discarded).
- The probe file uses standard `#include <stdio.h>` and conditional macros that
  `ccompiler` can handle.
- cmake will report the compiler as *"Unknown"* (since it does not recognise the
  ClaudeC99 compiler ID) but will still proceed and successfully build the
  project.

If cmake's link step passes `-fPIE` as a linker flag (`cc99 -fPIE … -o prog`),
`cc99` discards it and invokes `gcc -no-pie` as usual, which is correct for
our architecture.

---

## Bootstrap constraint

All `src/compiler.c` additions use only `strncmp`, `strcmp`, string
comparisons, array indexing, and `realloc` — all valid under the C89/C99 subset
the self-hosted compiler handles.  No VLAs, no `//` comments, no compound
literals.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Version

Update `src/version.c`:

```c
#define VERSION_STAGE  "01720000"
```

---

## Implementation order

1. Add `-std=…`/`-ansi` branch to `src/compiler.c` (accept, ignore).
2. Add `-isystem <dir>` branch to `src/compiler.c` (treat as `-I`).
3. Add `-w` branch to `src/compiler.c` (accept, no-op).
4. Update the `--help` output and usage string in `src/compiler.c`.
5. Fix `cc99`: change `-O0|-O1|-g` to `-O0|-O1|-O2|-g` and add the
   `-O3|-Os|-Og|-Ofast` drop arm.
6. Add `-std=*|-ansi` forwarding arm to `cc99`.
7. Add `-w` forwarding arm to `cc99`.
8. Add `-Wno-*` discard arm to `cc99`.
9. Add PIC/PIE discard arms to `cc99`.
10. Add hardening/code-generation discard arms to `cc99`.
11. Add machine-tuning discard arms to `cc99`.
12. Add `-isystem` forwarding arm to `cc99`.
13. Add dependency-tracking discard arms (`-MD`, `-MP`, `-MF`, `-MT`, `-MQ`)
    to `cc99`.
14. Add `-pthread` arm to `cc99` (appends `-lpthread` to `link_flags`).
15. Update the `--help` block in `cc99`.
16. Create `test/build_tool/run_tests.sh` (executable).
17. Create `test/build_tool/test_make_hello/` with all companion files
    (make `run_test.sh` executable).
18. Create `test/build_tool/test_make_cflags/` with all companion files
    (make `run_test.sh` executable).
19. Create `test/build_tool/test_cmake_hello/` with all companion files
    (make `run_test.sh` executable and add `.require`).
20. Register the `build_tool` runner in `test/run_all_tests.sh` inside the
    Linux x86_64 block.
21. Run `./test/run_all_tests.sh` — all pre-existing tests pass; `build_tool`
    suite passes (cmake test may be skipped if cmake not installed).
22. Run `./build.sh --mode=self-host` — C0→C1→C2 verified.
23. Bump `VERSION_STAGE` to `"01720000"` in `src/version.c`.
