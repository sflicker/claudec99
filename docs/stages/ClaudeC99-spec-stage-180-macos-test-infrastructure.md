# ClaudeC99 Stage 180 — macOS-Aware Test Infrastructure (Phase 0C)

## Goal

Make the full portable test suite and the C0→C1→C2 self-host cycle run on a
macOS ARM64 host through the Stage 179 `x86_64-macos` target (Rosetta 2),
while leaving the Linux x86-64 path unchanged.

After Stage 179 the compiler produces working Mach-O output, but nothing on
macOS can verify it. On the macOS host, `./test/run_all_tests.sh` reports
250 passed / 1911 failed. Three host problems cause this:

1. **No `timeout` command on macOS.** Every suite runner and `build.sh` call
   `timeout`. On macOS the call fails, so each test is reported as a
   "compiler error".
2. **Runners hardcode ELF/Linux tools:** `nasm -f elf64`, `gcc -no-pie` and
   `cc -no-pie`.
3. **The stub headers are glibc-only, and ccompiler can't detect macOS.**
   - `test/include/stdio.h` declares `stdin`/`stdout`/`stderr`, while macOS
     libc exports `__stdinp`/`__stdoutp`/`__stderrp`.
   - `test/include/errno.h` uses `__errno_location()`, while macOS uses
     `__error()`.
   - ccompiler always predefines `__linux__` and never `__APPLE__`, so a
     header cannot select the right names.

   This makes 4 `test/valid` programs fail to link on macOS. It also blocks
   a macOS self-host, because the compiler's own sources use `stderr`.

Problem 3 also fixes a Stage 179 limitation. Today a self-hosted ccompiler
built on macOS auto-detects the Linux target, because `__APPLE__` is never
defined during self-compilation.

---

## Part A — Shared platform helper: `test/lib/platform.sh` (new)

The suite runners source one helper file. It defines the following (bash):

```bash
HOST_OS=$(uname -s)    # Linux | Darwin
HOST_ARCH=$(uname -m)  # x86_64 | arm64

# Code generation target. Override with TARGET=<name> in the environment.
if [ -z "$TARGET" ]; then
    if [ "$HOST_OS" = "Darwin" ]; then TARGET=x86_64-macos; else TARGET=x86_64-linux; fi
fi
COMPILER_TARGET_FLAG="--target=$TARGET"

case "$TARGET" in
    x86_64-linux) NASM_FMT=elf64;   LINK_CMD=(gcc -no-pie) ;;
    x86_64-macos) NASM_FMT=macho64; LINK_CMD=(clang -arch x86_64) ;;
    arm64-macos)  echo "error: TARGET=arm64-macos not supported until Stage 181" >&2; exit 1 ;;
    *)            echo "error: unknown TARGET '$TARGET'" >&2; exit 1 ;;
esac

# Portable timeout: timeout -> gtimeout -> perl alarm fallback.
run_timeout() {   # usage: run_timeout <seconds> <cmd> [args...]
    if command -v timeout >/dev/null 2>&1; then timeout "$@"
    elif command -v gtimeout >/dev/null 2>&1; then gtimeout "$@"
    else perl -e 'alarm shift; exec @ARGV or exit 127' "$@"
    fi
}
```

How the perl fallback's exit status maps to `timeout`'s: on expiry,
`timeout` exits 124. The perl fallback dies by `SIGALRM`, which bash reports
as 142. No runner checks for 124 specifically today. The implementation must
confirm this still holds and note it in a comment in `platform.sh`.

---

## Part B — Runner changes

Each runner below sources `"$PROJECT_DIR/test/lib/platform.sh"` and then:

- replaces every `timeout` call with `run_timeout`
- passes `"$COMPILER_TARGET_FLAG"` on every ccompiler invocation that
  produces `.asm` for assembly
- uses `nasm -f "$NASM_FMT"` and `"${LINK_CMD[@]}"` instead of the
  hardcoded ELF tools

| Runner | Change |
|--------|--------|
| `test/valid/run_tests.sh`, `test/valid/run_test.sh` | timeout, target flag, nasm format, linker |
| `test/integration/run_tests.sh`, `test/integration/run_test.sh` | timeout, target flag, nasm format, linker (`cc -no-pie` → `${LINK_CMD[@]}`) |
| `test/invalid/run_tests.sh` | timeout only |
| `test/print_ast/run_tests.sh`, `test/print_tokens/run_tests.sh` | timeout only |
| `test/print_asm/run_tests.sh` | timeout, plus **always** `--target=x86_64-linux` (see below) |
| `test/unit/run_tests.sh` | no change (already passes on macOS) |

**`print_asm` is pinned to Linux.** Its `.expected` files are exact Linux
assembly text. The suite checks text only and never assembles, so it must
compare against the Linux target on every host. Pinning keeps one set of
expected files.

**Linux-gated suites are unchanged.** The `run_all_tests.sh` gating of the
system-include, optional-library and build-tool suites (Linux x86_64 only)
stays as it is.

**DWARF:** `test/integration` passes `-g -F dwarf` to nasm for `-g` tests.
The implementation must confirm that `nasm -f macho64 -g -F dwarf` is
accepted. If it is not, use the matching macho64 debug format, or skip
`test_dwarf_debug` on macOS with a `SKIP` line. Record which option was
taken in the milestone.

---

## Part C — Target-dependent predefined macros

### `include/preprocessor.h` / `src/preprocessor.c`

Add a target setter instead of changing the existing `preprocess*` signatures,
which unit tests call directly:

```c
#include "target.h"

/* Stage 180: select target-dependent predefined macros.
 * Default: TARGET_X86_64_LINUX. */
void preprocess_set_target(Target target);
```

In the predefined-macro block (`src/preprocessor.c`, "Target/ABI predefined
macros"):

| Macro | `x86_64-linux` | `x86_64-macos` |
|-------|----------------|----------------|
| `__x86_64__`, `__LP64__`, `_LP64`, `__CHAR_BIT__`, `__SIZEOF_*__` | defined | defined (unchanged) |
| `__linux__` | defined | **not defined** |
| `__unix__` | defined | **not defined** (clang on macOS does not define it) |
| `__APPLE__`, `__MACH__` | not defined | **defined as `1`** |

### `src/compiler.c`

Call `preprocess_set_target(target)` after target resolution and before the
per-file compile loop.

This also completes the Stage 179 auto-detect. When C1 is built with
`--target=x86_64-macos`, the preprocessor now sees `__APPLE__`, so the
`#if defined(__APPLE__)` default in `compiler.c` selects `x86_64-macos` in the
self-hosted binary.

---

## Part D — Stub header updates (`test/include/`)

### `stdio.h`

```c
#if defined(__APPLE__)
extern FILE *__stdinp;
extern FILE *__stdoutp;
extern FILE *__stderrp;
#define stdin  __stdinp
#define stdout __stdoutp
#define stderr __stderrp
#else
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
#endif
```

### `errno.h`

```c
#if defined(__APPLE__)
int *__error(void);
#define errno (*__error())
#else
int *__errno_location(void);
#define errno (*__errno_location())
#endif
```

The `E*` constant values above are the same on Linux and macOS, so they don't
change. The implementation must check every other macro and constant in
`test/include` that the suites use against the macOS SDK. Any value that
differs and is exercised by a test (for example `<fcntl.h>` flags, `<signal.h>`
numbers or `SEEK_*`) gets an `__APPLE__` branch. Values no test exercises are
left alone and listed in the milestone.

---

## Part E — `build.sh`

- Replace the two `timeout` calls with a `run_timeout` function, defined
  inline in `build.sh` with the same timeout → gtimeout → perl fallback as
  Part A. `build.sh` does not source `test/lib/platform.sh`.
- No other changes; Stage 179 already made bootstrap host-aware.

With Parts A–E in place, `./build.sh --mode=self-host` should complete on
macOS. C0 is AppleClang-built there; on Linux it stays GCC-built.

---

## Part F — `src/version.c`

`VERSION_STAGE "01800000"`.

---

## Tests

- **New unit or print-tokens test:** check that `__APPLE__` is defined and
  `__linux__` is not under `--target=x86_64-macos`, and the reverse under
  `--target=x86_64-linux`. Use whichever existing suite can pass a target
  flag. If none can, add a small `test/valid/preprocessor` pair that returns a
  sentinel exit code from `#if defined(...)` branches. Its expected value
  must hold on both hosts: test it under the host's default target, and
  name it so its exit code is the same everywhere.
- **No changes to existing tests.**

---

## Acceptance criteria

1. **Linux x86_64:**
   - `./test/run_all_tests.sh` totals are identical to Stage 179: 2161
     portable, plus the system-include, optional-library and build-tool
     counts, plus any tests added this stage.
   - `./build.sh --mode=self-host` passes C0→C1→C2.
   - Generated `.asm` for every test source is byte-identical to Stage 179
     under the Linux target, except the `__TIME__` test.
2. **macOS ARM64 (Rosetta 2):**
   - `./test/run_all_tests.sh` passes the full portable suite.
   - Any remaining failure is listed in the milestone with its root cause
     and an explicit follow-up. It must not be silently skipped.
   - `./build.sh --mode=self-host` completes C0→C1→C2 with tests passing at
     each step.
3. **Self-compilation report:** `docs/self-compilation-report.md` records
   both hosts' C0/C1/C2 results. If the Linux run can't be done from the
   implementing host, record it as pending.

---

## Roadmap deviations

- **`ARM64_TIER` is deferred to Stage 181.** The roadmap lists it under
  Phase 0C, but there is no ARM64 backend to exercise it yet. Its tier 1
  ("simple `statements/` — return only") also doesn't match the tree:
  `test/valid/statements/` holds only 6 switch tests. Stage 181 will define
  the tier-1 test set alongside the ARM64 skeleton. Stage 180 only reserves
  `TARGET=arm64-macos`, which errors in `platform.sh`.
- **Stub-header and predefined-macro work (Parts C and D) is added to Phase
  0C.** The roadmap did not list it, but neither the full macOS suite nor a
  macOS self-host is possible without it.

## Out of scope

- `bin/cc99` macOS support (its own platform/linker logic)
- Running the Linux-gated suites (system-include, optional-library,
  build-tool) on macOS
- ARM64 codegen and `ARM64_TIER` (Stage 181+)
- Any compiler language or codegen change beyond the predefined macros in
  Part C
