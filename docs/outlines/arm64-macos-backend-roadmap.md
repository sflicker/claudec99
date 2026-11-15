# ARM64/macOS Backend Roadmap

## Overview

Add Apple M1 (ARM64/AArch64) support to ClaudeC99 in three sequential goals:

1. **Phase 0A** — Build and run the compiler binary itself on macOS ARM64 (native host)
2. **Phase 0B** — Add an `x86_64-macos` cross-compile target so the ARM64-hosted compiler generates x86-64 Mach-O binaries that run under Rosetta 2 (minimal change to existing codegen)
3. **Phases 1–10** — Build the native ARM64 backend incrementally, unlocking test categories at each phase

The front-end (lexer, parser, AST, optimizer, preprocessor) is pure C99 standard library. It needs **no changes** for any of this work.

---

## Phase 0A — Build the Compiler on macOS ARM64 ✅ Stage 178

**Goal:** `bin/ccompiler` builds and runs on macOS ARM64.

The compiler source uses only standard C99 headers (`stdio.h`, `stdlib.h`, `string.h`,
`ctype.h`, `limits.h`, `setjmp.h`, `stdarg.h`, `stddef.h`, `time.h`, `errno.h`) — no
Linux-specific headers. Expected to build with zero or minimal changes:

```
cmake -S . -B build
cmake --build build
```

Work items:
- Verify `CMakeLists.txt` works with Apple Clang; fix any flag incompatibilities
- Fix any warnings Apple Clang raises that GCC does not (`-Wall -Wextra -pedantic`)
- Verify `bin/ccompiler` runs and produces `.asm` output for a test file

The generated assembly is still x86-64 Linux NASM at this point — it cannot be
assembled on macOS yet. The compiler binary itself is now native ARM64.

**Tests unlocked:** `print_tokens/`, `print_ast/`, `invalid/` suites (front-end only,
no assembly step), and integration tests with `.error` files (compile-error checks).

---

## Phase 0B — x86-64 Mach-O Cross-Compile Target ✅ Stage 179

**Goal:** ARM64-hosted compiler emits x86-64 Mach-O binaries that run under Rosetta 2.

This is a surface-level target variant — same x86-64 instructions and register model,
different symbol conventions and assembler invocation.

### Differences: ELF64 (Linux) vs Mach-O64 (macOS)

| Aspect              | Linux (current)         | macOS (new)                        |
|---------------------|-------------------------|------------------------------------|
| C symbol prefix     | `main`, `printf`        | `_main`, `_printf`                 |
| Assemble command    | `nasm -f elf64`         | `nasm -f macho64`                  |
| Link command        | `gcc -no-pie foo.o -o`  | `clang -arch x86_64 foo.o -o`      |
| Static data ref     | `[rel varname]`         | `[rel _varname]` (with underscore) |
| Uninit globals      | `section .bss; resb N`  | same (NASM macho64 supports it)    |
| DWARF               | NASM `%line` directives | same                               |

### Implementation

- Add `TARGET_X86_64_LINUX`, `TARGET_X86_64_MACOS`, `TARGET_ARM64_MACOS` to a new
  `include/target.h`
- Add `--target=x86_64-macos` / `--target=arm64-macos` flags to `compiler.c`; auto-detect
  host platform (`uname -s`/`uname -m`) when flag is omitted
- In `codegen.c`: conditionalize symbol emission — when target is `X86_64_MACOS`, prefix
  all C-visible names with `_`
- Update `build.sh` to use `nasm -f macho64` and `clang -arch x86_64` on macOS
- Pass `--target` through to the compiler in `build.sh`

**Tests unlocked:** All existing valid/integration run tests via Rosetta 2. This path
serves as a correctness baseline while the native ARM64 backend is built.

---

## Phase 0C — macOS-Aware Test Infrastructure ✅ Stage 180

**Goal:** Test runners detect platform, select the right assembler/linker, and support
progressive ARM64 test enablement.

### Test suite categories by what they require

| Suite                        | Requires backend | Requires binary execution | Status on macOS day one      |
|------------------------------|------------------|--------------------------|------------------------------|
| `print_tokens/`              | No               | No                       | Works immediately            |
| `print_ast/`                 | No               | No                       | Works immediately            |
| `invalid/`                   | No               | No                       | Works immediately            |
| `print_asm/`                 | Yes (text check) | No                       | Phase 0B+; needs ARM64 expected files for Phase 1+ |
| Compile-error `integration/` | No               | No                       | Works immediately            |
| `valid/`                     | Yes              | Yes                      | Phase 0B (Rosetta) or Phase 1+ (ARM64 native) |
| Run-requiring `integration/` | Yes              | Yes                      | Same as above                |

### Changes to test runners

- Detect platform: `uname -s` / `uname -m` to identify `Darwin arm64`
- On macOS: replace `nasm -f elf64` with `nasm -f macho64`; replace `gcc -no-pie` with
  `clang -arch x86_64` (Phase 0B path) or `clang -arch arm64` (ARM64 backend path)
- Add `TARGET` env var (`x86_64-macos`, `arm64-macos`) so runners invoke the compiler
  with `--target $TARGET`
- Add `ARM64_TIER` env var (0–10) that controls which `test/valid/` subdirectories are
  included for ARM64 backend testing — the runner skips categories beyond the current tier
- Keep the existing Linux path unchanged

**Stage 180 scope (see `docs/stages/ClaudeC99-spec-stage-180-macos-test-infrastructure.md`):**
- Adds: shared `test/lib/platform.sh` (`TARGET`, `NASM_FMT`, `LINK_CMD`, portable
  `run_timeout` — macOS has no `timeout`); `print_asm` pinned to `--target=x86_64-linux`;
  target-dependent predefined macros (`__APPLE__`/`__MACH__` vs `__linux__`/`__unix__`);
  `__APPLE__` branches in `test/include` `stdio.h`/`errno.h`; macOS self-host via `build.sh`.
- Defers: `ARM64_TIER` gating to Stage 181 (no ARM64 backend to exercise yet; tier 1
  "simple `statements/`" does not match the tree — `test/valid/statements/` holds only
  6 switch tests).
- Also added during implementation: Mach-O GOT access for `extern` data objects
  (dylib data such as `__stdoutp` is not RIP-reachable). Result: 2163/2163 portable
  tests and C0→C1→C2 self-host pass on macOS ARM64 via Rosetta 2.

### Tier-to-category mapping

| Tier | Newly unlocked `test/valid/` categories                              |
|------|----------------------------------------------------------------------|
| 1    | Manifest-listed `return <literal>` programs (`test/valid/arm64_tiers.txt`) |
| 2    | `arithmetic/`, `assignment/`, `expressions/`                         |
| 3    | `comparisons/`, `control_flow/`, full `statements/`                  |
| 4    | `functions/`                                                         |
| 5    | `strings/`, global `declarations/`                                   |
| 6    | `pointers/`, `arrays/`, `structs/`                                   |
| 7    | `chars/`, `casting/`, `bitwise/`                                     |
| 8    | `floating_point/`                                                    |
| 9    | `varargs/`, `stdlib/`                                                |
| 10   | `enums/`, `typedefs/`, `unions/`, `misc/`, `preprocessor/`, full `integration/` |

Since Stage 181 this mapping is enforced by the manifest `test/valid/arm64_tiers.txt`
(`<tier> <file-or-dir>` lines). Each phase adds its tier's entries as it lands. Under
`TARGET=arm64-macos` the `integration/` suite is skipped entirely until tier 10.

---

## Phase 1 — ARM64 Codegen Skeleton ✅ Stage 181

**New files:** `src/codegen_arm64.c`, `include/codegen_arm64.h`

**Goal:** Compile `int main(void) { return 0; }` to a working native ARM64 binary.

A `CodeGenArm64` struct mirrors `CodeGen` but substitutes ARM64-specific fields.

### Key decisions

**Symbol prefix:** Mach-O requires a leading underscore on all C names
(`main` → `_main`, `printf` → `_printf`).

**Sections:** `.text`, `.data`, `.section __TEXT,__cstring` — not NASM `section`.
Data directives: `.byte`, `.short`, `.long`, `.quad`, `.asciz` (not `db`/`dw`/`dd`/`dq`).

**Frame layout (AAPCS64):**
```
stp x29, x30, [sp, #-N]!    ; save frame pointer + link register, grow stack
mov x29, sp                  ; set frame pointer
; locals addressed as [x29, #-offset]
; stack size N must be a multiple of 16
ldp x29, x30, [sp], #N      ; epilogue: restore and shrink stack
ret
```

**Assemble/link:** `clang -arch arm64 foo.s -o foo`

**Tests unlocked:** `ARM64_TIER=1` — the `return <literal>` programs listed in
`test/valid/arm64_tiers.txt`.

**Stage 181 scope (see `docs/stages/ClaudeC99-spec-stage-181-arm64-skeleton.md`):**
- `src/codegen_arm64.c` / `include/codegen_arm64.h`; `--target=arm64-macos` emits `<name>.s`
  (Apple/GNU syntax, assembled and linked with `clang -arch arm64`).
- `ARM64_TIER` gating (deferred from Phase 0C) uses a manifest, `test/valid/arm64_tiers.txt`
  (`<tier> <file-or-dir>` lines), instead of a hardcoded directory table, because tier 1
  cannot be a whole directory. `test/lib/platform.sh` gains `ASM_EXT`, `assemble` and
  `arm64_tier_includes`.
- `arm64-macos` predefines `__aarch64__`/`__arm64__` (not `__x86_64__`). macOS auto-detect
  stays `x86_64-macos` until Phase 10.
- Result: `CodeGenArm64` is a minimal independent struct (`output` only), and later phases
  add fields as needed. Phase 1 accepts only a function body that is a single
  `return <integer literal 0..65535>`, and rejects anything else with
  `arm64-macos: <construct> not yet supported`. `TARGET=arm64-macos` tier 1 passes 5/5
  natively (the first native Apple Silicon binaries). The default macOS suite passes
  2166/2166, and the C0→C1→C2 self-host passes on macOS.

---

## Phase 2 — Integer Arithmetic and Local Variables

**Goal:** Compile arithmetic expressions and local variable reads/writes.

### Instruction mapping

| Operation      | ARM64                                              |
|----------------|----------------------------------------------------|
| add / sub      | `add x0, x1, x2` or `add x0, x1, #imm`            |
| multiply       | `mul x0, x1, x2`                                   |
| signed divide  | `sdiv x0, x1, x2`                                  |
| remainder      | `sdiv x2, x0, x1` then `msub x0, x2, x1, x0`      |
| negate         | `neg x0, x1`                                       |
| bitwise NOT    | `mvn x0, x1`                                       |
| AND / OR / XOR | `and` / `orr` / `eor`                              |
| shifts         | `lsl` / `lsr` / `asr`                              |

**Large immediates:** constants that do not fit in a 12-bit immediate need a
`movz`/`movk` sequence. Add an `emit_load_imm()` helper that selects the right form
based on the constant value.

**Local variable access:** `ldr x0, [x29, #-8]` / `str x0, [x29, #-8]`.

**Tests unlocked:** `ARM64_TIER=2` — `arithmetic/`, `assignment/`, `expressions/`.

**Stage 182 scope (see `docs/stages/ClaudeC99-spec-stage-182-arm64-arithmetic-locals.md`):**
- Signed `int`/`long` locals (pre-scanned frame slots, scope stack for nested blocks and
  shadowing), `+ - * / %`, `& | ^ << >>`, unary `- ~ +`, assignment (compound forms come
  pre-lowered from the parser), `++`/`--`, comma, `movz`/`movk` constants, and an
  epilogue label (implicit `return 0` from `main`). Expressions use stack-machine
  evaluation: result in `w0`/`x0`, operands pushed with `str x0, [sp, #-16]!`.
- Tier 2 is **file-level, not whole directories**. AST analysis found only 65 of the 166
  tests in these three directories use Phase 2 constructs. The rest need comparisons
  (Phase 3), calls (Phase 4), pointers, arrays and structs (Phase 6), or char/short/FP
  (Phases 7–8). They join the manifest when those phases land.

---

## Phase 3 — Comparisons and Control Flow

**Goal:** Compile if/else, while, for, do-while, break, continue, switch, goto.

- Comparison: `cmp x0, x1` then `cset x0, eq` (or `ne`/`lt`/`le`/`gt`/`ge`/`lo`/`hi`)
- Conditional branch: `b.eq`, `b.ne`, `b.lt`, `b.le`, `b.gt`, `b.ge` to labels
- Unconditional: `b label`
- Local labels: `L_else_0:` etc. (Mach-O `L`-prefixed labels are non-exported)
- Short-circuit zero/non-zero: `cbz x0, label` / `cbnz x0, label`

**Tests unlocked:** `ARM64_TIER=3` — `comparisons/`, `control_flow/`, full `statements/`.

---

## Phase 4 — Functions and Calls

**Goal:** Compile multi-parameter functions, direct and indirect calls.

### AAPCS64 calling convention

- Integer args: `x0`–`x7` (first 8); overflow args on stack
- Float args: `v0`–`v7` (first 8); overflow args on stack
- Return value: `x0` (integer), `v0` (float/double)
- Callee-saved: `x19`–`x28`, `x29` (frame pointer), `x30` (link register)
- `x30` must be saved in every non-leaf function (the `stp x29, x30` prologue handles this)
- **Stack must be 16-byte aligned at every `bl` instruction** — Apple Silicon enforces this

### Call sequences

- Direct call: `bl _funcname`
- Indirect (function pointer): `blr x0`
- Stack overflow args: pushed right-to-left below the caller's frame before `bl`

**Tests unlocked:** `ARM64_TIER=4` — `functions/`.

---

## Phase 5 — Global Variables and String Literals

**Goal:** Compile global variable reads/writes and string literal emission.

Mach-O has no RIP-relative `[rel symbol]` syntax. All global symbol references use a
two-instruction page-relative sequence:

```
adrp x0, _varname@PAGE          ; load page address into x0
add  x0, x0, _varname@PAGEOFF   ; add page offset
ldr  x1, [x0]                   ; load from that address (or omit add for address-of)
```

This two-instruction pair replaces every `[rel symbol]` and `lea rax, [rel symbol]`
in the existing x86-64 backend.

### Data layout

| Data                 | Mach-O directive                                     |
|----------------------|------------------------------------------------------|
| Initialized globals  | `.data` section with `.quad`/`.long`/`.short`/`.byte`|
| Uninitialized globals| `.comm _varname, size, align`                        |
| String literals      | `.section __TEXT,__cstring,cstring_literals` + `.asciz` |

**Tests unlocked:** `ARM64_TIER=5` — `strings/`, global `declarations/`.

---

## Phase 6 — Pointers, Arrays, and Structs

**Goal:** Compile address-of, pointer dereference, array indexing, struct member access.

- Address-of local variable: `add x0, x29, #-offset`
- Pointer dereference: `ldr x0, [x0]`
- Indexed array access: `ldr x0, [x1, x2, lsl #3]` (scale by element size using `lsl`)
- Struct member access: fixed positive offsets, same layout as x86-64 (no ABI difference
  for structs in memory on ARM64)

**Tests unlocked:** `ARM64_TIER=6` — `pointers/`, `arrays/`, `structs/`.

---

## Phase 7 — Type-Width Loads and Sign Extension

**Goal:** Correctly load/store `char`, `short`, `int`, `long`, pointers at their natural widths.

ARM64 has explicit-width load/store for all sizes. Writing a `w` register
(32-bit view of `x`) automatically zero-extends to the full 64-bit `x` register.

| C type    | Unsigned load  | Sign-extending load | Store    |
|-----------|----------------|---------------------|----------|
| `char`    | `ldrb w0, [x1]`| `ldrsb x0, [x1]`    | `strb w0`|
| `short`   | `ldrh w0, [x1]`| `ldrsh x0, [x1]`    | `strh w0`|
| `int`     | `ldr w0, [x1]` | `ldrsw x0, [x1]`    | `str w0` |
| `long`/ptr| `ldr x0, [x1]` | —                   | `str x0` |

**Tests unlocked:** `ARM64_TIER=7` — `chars/`, `casting/`, `bitwise/`.

---

## Phase 8 — Float and Double

**Goal:** Compile float/double arithmetic, comparisons, and conversions.

ARM64 floating-point uses NEON/VFP registers: `s0`–`s31` (float), `d0`–`d31` (double).

- Float args/return: `v0`–`v7` (via `s0`–`s7` / `d0`–`d7`), not `x0`–`x7`
- Arithmetic: `fadd s0, s1, s2`, `fsub`, `fmul`, `fdiv`
- Compare: `fcmp s0, s1` then `b.eq`/`b.ne` etc.
- Load/store: `ldr s0, [x29, #-4]`, `ldr d0, [x29, #-8]`
- Integer ↔ float: `scvtf d0, x0` (int→double), `fcvtzs x0, d0` (double→int, truncate)
- Float ↔ double: `fcvt s0, d0` (double→float), `fcvt d0, s0` (float→double)

**Note:** The peephole optimizer (`-O2`) is x86-64 specific text-pattern matching. On the
ARM64 backend, `-O2` is a no-op until a separate ARM64 peephole pass is added later.

**Tests unlocked:** `ARM64_TIER=8` — `floating_point/`.

---

## Phase 9 — Variadic Functions

**Goal:** Compile `printf`, `scanf`, and user-defined variadic functions.

Apple's ARM64 ABI simplifies `va_list`: it is essentially a `char *` pointing to the
next argument on the stack. Named args occupy `x0`–`x7` / `v0`–`v7`; overflow args are
on the stack in memory. This is significantly simpler than the x86-64 register save area
model (no `reg_save_area`, `gp_offset`, `fp_offset`, 176-byte save area).

The `va_start` / `va_arg` / `va_end` macro expansion in the preamble must be reworked
for the ARM64 ABI. External variadic calls (`printf`, `scanf`) use `bl _printf` etc.
with args in `x0`–`x7` / `v0`–`v7` as normal.

**Tests unlocked:** `ARM64_TIER=9` — `varargs/`, `stdlib/`.

---

## Phase 10 — Full Test Suite and Self-Host

**Goal:** All test categories pass on ARM64; compiler bootstraps itself.

Set `ARM64_TIER=10` and run the full suite including `enums/`, `typedefs/`, `unions/`,
`misc/`, `preprocessor/`, and all `integration/` run tests. Fix any remaining gaps.

Then run the C0→C1→C2 self-hosting bootstrap with the ARM64 backend:
- Compile all compiler source files using itself with `--target arm64-macos`
- Assemble with `clang -arch arm64`
- Link and verify output matches the Clang-built baseline

---

## Implementation Progress

Each phase has a spec document in `docs/stages/` and is checked off here when both
spec and implementation are committed.

| Phase | Stage | Key deliverable                                      | Test tier unlocked                            | Done |
|-------|-------|------------------------------------------------------|-----------------------------------------------|------|
| 0A    | 178   | Compiler binary runs natively on M1                  | `print_tokens/`, `print_ast/`, `invalid/`     | [x]  |
| 0B    | 179   | `x86_64-macos` target, Rosetta 2 execution           | All existing tests via Rosetta                | [x]  |
| 0C    | 180   | macOS-aware test runners, macOS stub headers         | Infrastructure only                           | [x]  |
| 1     | 181   | ARM64 skeleton + `ARM64_TIER` gating                 | Tier 1 — simple return                        | [x]  |
| 2     | 182   | Arithmetic, local variables                          | Tier 2 — arithmetic/assignment/expressions    | [ ]  |
| 3     | 183   | Control flow                                         | Tier 3 — comparisons/control_flow             | [ ]  |
| 4     | 184   | Functions and calls                                  | Tier 4 — functions                            | [ ]  |
| 5     | 185   | Globals, string literals                             | Tier 5 — strings/declarations                 | [ ]  |
| 6     | 186   | Pointers, arrays, structs                            | Tier 6 — pointers/arrays/structs              | [ ]  |
| 7     | 187   | Type-width loads, sign extension                     | Tier 7 — chars/casting/bitwise                | [ ]  |
| 8     | 188   | Float and double                                     | Tier 8 — floating_point                       | [ ]  |
| 9     | 189   | Variadic functions, printf                           | Tier 9 — varargs/stdlib                       | [ ]  |
| 10    | 190   | Full test suite + ARM64 self-host                    | Tier 10 — complete                            | [ ]  |

**Next up:** Phase 2 (stage 182) — ARM64 integer arithmetic and local variables.

### Open items (as of Stage 181)

- **Linux self-host pending.** Stages 180–181 were self-hosted on macOS ARM64 only.
  Linux-target `.asm` was verified byte-identical to the previous stage instead.
- **Error line numbers off by ~5.** Compile errors in both backends report lines about
  5 too high (e.g. `file.c:7` for line 2), likely from the injected preamble.
- **macOS `ld` warnings (harmless).** `pointer not aligned`: data pointer tables are not
  8-byte aligned. `no platform load command`: NASM Mach-O objects lack
  `LC_BUILD_VERSION`.
- **`bin/cc99`** has no macOS/ARM64 support yet.

The Rosetta 2 path (Phase 0B) gives a fully working compiler on M1 with no new ISA
work, acting as a correctness baseline while the native ARM64 backend is built phase
by phase.
