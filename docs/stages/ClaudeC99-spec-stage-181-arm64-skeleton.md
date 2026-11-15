# ClaudeC99 Stage 181 — ARM64 Codegen Skeleton + `ARM64_TIER` Gating (Phase 1)

## Goal

Start the native ARM64 (AArch64) backend for macOS on Apple Silicon. When this
stage is done:

- `--target=arm64-macos` compiles `int main(void) { return 42; }` into Apple
  ARM64 assembly. `clang -arch arm64` assembles and links it, and it runs
  natively (no Rosetta) with exit code 42.
- The test infrastructure can run `test/valid` against the ARM64 backend one
  tier at a time (`ARM64_TIER`), starting with tier 1: programs whose `main`
  is a bare `return <integer-literal>;`.

The `x86_64-linux` and `x86_64-macos` targets must not change at all.

---

## Part A — ARM64 backend files (new)

### `include/codegen_arm64.h`

```c
#ifndef CCOMPILER_CODEGEN_ARM64_H
#define CCOMPILER_CODEGEN_ARM64_H

#include <stdio.h>
#include "ast.h"

/* Stage 181: ARM64 (AArch64) Apple/GNU-syntax code generator for the
 * arm64-macos target.  Phase 1 skeleton — see docs/outlines/
 * arm64-macos-backend-roadmap.md for the phased plan. */
typedef struct {
    FILE *output;
} CodeGenArm64;

void codegen_arm64_init(CodeGenArm64 *cg, FILE *output);
void codegen_arm64_translation_unit(CodeGenArm64 *cg, ASTNode *tu);
void codegen_arm64_free(CodeGenArm64 *cg);

#endif
```

`CodeGenArm64` starts with only `output`. Later phases add fields (locals,
labels, string pool, …) as they need them. It must not reuse or share
state with `CodeGen`.

### `src/codegen_arm64.c`

**Supported input (Phase 1):** a translation unit whose top-level nodes are:

- `AST_TYPEDEF_DECL`: ignored. The built-in preamble always adds
  `__builtin_va_list` typedefs, and they need no codegen.
- `AST_FUNCTION_DECL` **with a body**: the body is an `AST_BLOCK` holding
  exactly one `AST_RETURN_STATEMENT`, whose expression is an
  `AST_INT_LITERAL`.
- `AST_FUNCTION_DECL` **without a body** (a prototype): ignored.

Anything else, at any level, is a compile error (exit 1):

```
error: arm64-macos: <construct> not yet supported
```

`<construct>` is a short readable name for the AST node type, e.g. `local
variable declaration`, `binary expression`, `function call`, `global
variable`. Use the existing node-type naming if one exists; otherwise a
small local switch is fine.

**Integer literal range:** the value must fit `mov w0, #imm`, i.e.
`0 ≤ value ≤ 65535`. Anything else gives `error: arm64-macos: integer
literal <v> out of range for this stage`. `movz`/`movk` sequences belong to
Phase 2.

**Output shape** for `int main(void) { return 42; }`:

```asm
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_main
	.p2align	2
_main:
	stp	x29, x30, [sp, #-16]!
	mov	x29, sp
	mov	w0, #42
	ldp	x29, x30, [sp], #16
	ret
```

- **Symbols:** C function names get the Mach-O `_` prefix. Every function
  with a body gets `.globl`, except one declared `static`, which gets no
  `.globl`.
- **Header:** the `.section` line is emitted once, at the top.
- **Frame:** the frame is the full AAPCS64 prologue/epilogue from the roadmap
  (`stp`/`mov x29, sp` … `ldp`/`ret`). It is emitted even for leaf functions,
  so later phases can extend it.
- **Formatting:** tabs between mnemonic and operands, matching Apple clang
  `-S` style. Exact whitespace is not part of the contract.

---

## Part B — `src/compiler.c`

1. **Accept the target.** Remove the "target 'arm64-macos' is not yet
   implemented" error. `--target=arm64-macos` is now a valid target. Auto-
   detection is unchanged: macOS still defaults to `x86_64-macos` until the
   ARM64 backend is complete (Phase 10).
2. **Output path.** For `arm64-macos`, the output file is `<name>.s`; the
   x86 targets keep `<name>.asm`. Add
   `make_output_path_ext(char *out, size_t n, const char *input, const char *ext)`
   to `include/util.h` / `src/util.c`, and make the existing
   `make_output_path` call it with `".asm"`. Its behaviour stays the same.
3. **Codegen dispatch.** In `compile_one_file`, `arm64-macos` uses
   `CodeGenArm64` (`codegen_arm64_init` / `_translation_unit` / `_free`), and
   every other target uses the existing `CodeGen` path unchanged.
4. **x86-only options under `arm64-macos`:**
   - `-O2`: the peephole pass is skipped (it is x86 text-pattern matching).
     `-O1` (AST-level) still runs.
   - `-g`: accepted and ignored (no debug directives are emitted). Emit no
     warning.
5. **`--help`:** list `arm64-macos` among the targets, marked
   "(experimental)".

## Part C — Predefined macros (`src/preprocessor.c`)

Extend `preprocess_set_target`'s predefined-macro selection:

| Macro | `x86_64-linux` | `x86_64-macos` | `arm64-macos` (new) |
|-------|----------------|----------------|---------------------|
| `__x86_64__` | ✓ | ✓ | **—** |
| `__aarch64__`, `__arm64__` | — | — | **✓** |
| `__linux__`, `__unix__` | ✓ | — | — |
| `__APPLE__`, `__MACH__` | — | ✓ | ✓ |
| `__LP64__`, `_LP64`, `__CHAR_BIT__`, `__SIZEOF_*__` | ✓ | ✓ | ✓ (same LP64 values) |

---

## Part D — Test infrastructure

### `test/lib/platform.sh`

- **`TARGET=arm64-macos`** is now valid, with no error. It requires a Darwin
  host; on any other host, error with `TARGET=arm64-macos requires a macOS
  host`.
- **New variable `ASM_EXT`:** `asm` for the x86 targets, `s` for
  `arm64-macos`.
- **New function `assemble <in> <out> [extra flags...]`,** which replaces the
  runners' direct `nasm -f "$NASM_FMT"` calls:
  - x86 targets: `nasm -f "$NASM_FMT" <extra flags> <in> -o <out>`
  - `arm64-macos`: `clang -arch arm64 -c <in> -o <out>`. Extra flags (the
    NASM `-g -F dwarf`) are ignored.
- **`LINK_CMD` for `arm64-macos`:** `(clang -arch arm64)`.
- **New variable `ARM64_TIER`:** defaults to `1`, overridable from the
  environment. It is only used when `TARGET=arm64-macos`.
- **New function `arm64_tier_includes <path-relative-to-test/valid>`:**
  returns 0 if the manifest has an entry with `tier ≤ $ARM64_TIER` that
  equals the path, or is a directory prefix of it.

### `test/valid/arm64_tiers.txt` (new manifest)

```
# ARM64 test tiers — see docs/outlines/arm64-macos-backend-roadmap.md.
# Format: <tier> <path relative to test/valid>   (a file, or a directory = all tests under it)
# A test runs under TARGET=arm64-macos when an entry with tier <= ARM64_TIER matches it.
1 misc/test_return__42.c
1 control_flow/test_return__42.c
1 arithmetic/test_octal_zero__0.c
1 misc/test_return_zero__0.c
1 misc/test_return_255__255.c
```

Blank lines and `#` comments are ignored. Later stages add entries for
their tiers.

### Runners

- **`test/valid/run_tests.sh`:**
  - Under `TARGET=arm64-macos`, skip every test not included by
    `arm64_tier_includes`.
  - Skipped tests are **not counted** in `total`, so the summary line stays
    `Results: P passed, F failed, T total`.
  - Print one line before the summary:
    `ARM64_TIER=<n>: <k> of <N> tests in scope`.
  - Use `ASM_EXT` and `assemble` instead of `.asm` and `nasm`.
- **`test/valid/run_test.sh`, `test/integration/run_tests.sh`,
  `test/integration/run_test.sh`, and the custom integration
  `run_test.sh` scripts:** use `ASM_EXT` and `assemble` instead of
  hardcoding `.asm` and `nasm`.
- **`test/integration/run_tests.sh` under `TARGET=arm64-macos`:** skip the
  whole suite. Print `SKIP  integration suite (TARGET=arm64-macos; enabled
  at tier 10)` and then `Results: 0 passed, 0 failed, 0 total`.
- **Unchanged:** `test/invalid`, `test/print_ast`, `test/print_tokens` (front
  end only) and `test/print_asm` (pinned to `x86_64-linux`).

With `TARGET` unset, all runner behaviour must be exactly as in Stage 180,
apart from the internal `assemble`/`ASM_EXT` refactor.

---

## Part E — `src/version.c`

`VERSION_STAGE "01810000"`.

---

## Tests

1. **New `test/valid/misc/test_return_zero__0.c`:**
   `int main(void) { return 0; }`
2. **New `test/valid/misc/test_return_255__255.c`:**
   `int main(void) { return 255; }`
3. **New `test/integration/test_arm64_skeleton_asm/`** (custom
   `run_test.sh`, host-independent and text-only, so it runs on Linux too):
   - Compile `return 42` with `--target=arm64-macos`.
   - Check that `<name>.s` exists, `<name>.asm` does not, and the output
     contains `.globl	_main`, `_main:`, `mov	w0, #42` and `ret`. Match on
     whitespace-normalized text.
   - Compile a program with a local variable under `--target=arm64-macos`.
     Expect exit 1 and the stderr substring `arm64-macos:` and `not yet
     supported`.
   - Compile `return 70000` under `--target=arm64-macos`. Expect the
     out-of-range error.
4. **New `test/integration/test_target_predefined_macros`:** extend the
   existing `run_test.sh` with an `arm64-macos` check: surviving
   identifiers are `has_apple has_mach` plus new markers for `__aarch64__`
   and `__arm64__`, and no `has_x86_64`. Add `has_aarch64` / `has_arm64`
   lines to `macros.c` (the x86 expectations gain nothing).
5. Tests 1 and 2 are target-neutral, so they also run (and pass) under
   both x86 targets. They join the default suite totals.

---

## Acceptance criteria

1. **Native ARM64 run on Apple Silicon:** on macOS ARM64,
   `TARGET=arm64-macos ./test/valid/run_tests.sh` reports
   `ARM64_TIER=1: 5 of <N> tests in scope` and `Results: 5 passed, 0 failed,
   5 total`.
2. **macOS default target (`x86_64-macos`):** `./test/run_all_tests.sh`
   passes the full portable suite with the new tests (2163 + 3 = 2166: two
   valid and one integration).
3. **Linux-target output is unchanged:** generated `.asm` for every test
   source under `--target=x86_64-linux` is byte-identical to Stage 180,
   except the `__TIME__` test and the new tests.
4. **Self-host:** `./build.sh --mode=self-host` (macOS, `x86_64-macos` path)
   passes C0→C1→C2. `src/codegen_arm64.c` is added to `build.sh`'s
   `SRC_FILES` and `CMakeLists.txt`, so it must self-compile.
5. **Linux self-host:** remains pending, as in Stage 180. Record the status.

---

## Roadmap updates (same commit as the spec)

- Phase 1 now includes `ARM64_TIER` gating, using the manifest-based design
  above.
- The tier-to-category table: tier 1 is "manifest-listed `return
  <literal>` programs" instead of "simple `statements/`".

## Out of scope

- Any ARM64 construct beyond `return <literal 0..65535>` (arithmetic,
  locals, `movz`/`movk`, control flow, calls, globals, …): Phases 2+
- Switching macOS auto-detection to `arm64-macos`: Phase 10
- An ARM64 peephole pass, and ARM64 DWARF / `-g`
- `print_asm` expected files for ARM64
- `bin/cc99` macOS/ARM64 support
- Fixing the macOS `ld` warnings (pointer alignment, platform load command)
