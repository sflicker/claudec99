# ClaudeC99 Stage 179 — x86-64 Mach-O Cross-Compile Target (Phase 0B)

## Goal

Add a `x86_64-macos` target so the ARM64-hosted compiler can emit x86-64 assembly
that assembles into Mach-O64 objects and runs under Rosetta 2 on Apple Silicon.

This is a surface-level change — same x86-64 instructions and register model as the
existing Linux backend. Only two things differ:

1. **C symbol names** must be prefixed with `_` in Mach-O (e.g. `main` → `_main`,
   `printf` → `_printf`, `myvar` → `_myvar`). Internal compiler-generated labels
   (`Lstr0`, `Lfc0`, `Lstatic_...`, `.L_usr_...`) are not C symbols and do **not**
   get this prefix.
2. The **`section .note.GNU-stack`** footer is an ELF-only marker and must be
   omitted on macOS.

Everything else — instruction encoding, sections (`.text`, `.data`, `.bss`,
`.rodata`), `[rel symbol]` addressing, DWARF `%line` directives — is accepted by
`nasm -f macho64` without change.

The `build.sh` assembler/linker invocation also changes on macOS.

---

## New file: `include/target.h`

```c
#ifndef CCOMPILER_TARGET_H
#define CCOMPILER_TARGET_H

typedef enum {
    TARGET_X86_64_LINUX,   /* default on Linux: ELF64/NASM */
    TARGET_X86_64_MACOS,   /* Mach-O64/NASM, runs under Rosetta 2 on M1 */
    TARGET_ARM64_MACOS     /* reserved for Phase 1+ */
} Target;

#endif
```

---

## Changes to `src/compiler.c`

### Add `--target` flag parsing

After the existing flag loop, add:

- `--target=x86_64-linux` → `TARGET_X86_64_LINUX`
- `--target=x86_64-macos` → `TARGET_X86_64_MACOS`
- `--target=arm64-macos`  → `TARGET_ARM64_MACOS` (accepted; ARM64 backend not yet
  implemented — compiler exits with an error for now)

**Auto-detection** when `--target` is omitted:

```c
#if defined(__APPLE__)
    target = TARGET_X86_64_MACOS;
#else
    target = TARGET_X86_64_LINUX;
#endif
```

### Pass target to codegen

After `codegen_init(&cg, out)`, add:

```c
cg.target_macho = (target == TARGET_X86_64_MACOS);
```

Add `target` to the `--help` output.

---

## Changes to `include/codegen.h`

Add one field to `CodeGen`:

```c
/* Stage 179: 1 when emitting Mach-O64 output (x86_64-macos target).
 * Affects C symbol prefix (_ required) and .note.GNU-stack suppression. */
int target_macho;
```

---

## Changes to `src/codegen.c`

### Add `sp()` helper

Add near the top of the file, after includes:

```c
/* Return the C symbol prefix for the current target:
 * "_" for Mach-O (macOS), "" for ELF (Linux). */
static const char *sp(const CodeGen *cg) {
    return cg->target_macho ? "_" : "";
}
```

### Symbol emission changes

Every `fprintf` that emits a C-visible symbol name must be updated to use `sp(cg)`.
The format changes from `"%s"` to `"%s%s"` (prefix + name).

Internal compiler labels (`Lstr`, `Lfc`, `Lfp_smask_*`, `Lfp_one_*`, `Lstatic_*`,
`Lcompound_*`, `.L_usr_*`) do **not** use `sp()` — they are never C-visible symbols.

**Category A — `global` and `extern` directives**

| Line | Current | Change |
|------|---------|--------|
| 6622 | `"global %s\n", node->value` | `"global %s%s\n", sp(cg), node->value` |
| 7562 | `"global %s\n", gv->name` | `"global %s%s\n", sp(cg), gv->name` |
| 7835 | `"global %s\n", gv->name` | `"global %s%s\n", sp(cg), gv->name` |
| 7099 | `"extern %s\n", c->value` | `"extern %s%s\n", sp(cg), c->value` |
| 7122 | `"extern %s\n", gi->name` | `"extern %s%s\n", sp(cg), gi->name` |

**Category B — label definitions**

| Line | Current | Change |
|------|---------|--------|
| 6623 | `"%s:\n", node->value` | `"%s%s:\n", sp(cg), node->value` |
| 7569 | `"%s: db ", gv->name` | `"%s%s: db ", sp(cg), gv->name` |
| 7581 | `"%s:\n", gv->name` | `"%s%s:\n", sp(cg), gv->name` |
| 7663 | `"%s:\n", gv->name` | `"%s%s:\n", sp(cg), gv->name` |
| 7748 | `"%s: dq %s\n", gv->name, anon` | `"%s%s: dq %s\n", sp(cg), gv->name, anon` |
| 7755 | `"%s: dq %s\n", gv->name, child->value` | `"%s%s: dq %s%s\n", sp(cg), gv->name, sp(cg), child->value` |
| 7768 | `"%s: dq %s\n", gv->name, arr_name` | `"%s%s: dq %s%s\n", sp(cg), gv->name, sp(cg), arr_name` |
| 7768+| `"%s: dq %s + %ld\n", gv->name, arr_name` | `"%s%s: dq %s%s + %ld\n", sp(cg), gv->name, sp(cg), arr_name` |
| 7792 | `"%s: dq %s\n", gv->name, anon` | `"%s%s: dq %s\n", sp(cg), gv->name, anon` |
| 7813 | `"%s: %s %s\n", gv->name, dir, gv->init_label` | `"%s%s: %s %s%s\n", sp(cg), gv->name, dir, sp(cg), gv->init_label` |
| 7817 | `"%s: %s %ld\n", gv->name, dir, val` | `"%s%s: %s %ld\n", sp(cg), gv->name, dir, val` |
| BSS  | `"%s: resb %d\n", gv->name` (3 instances) | `"%s%s: resb %d\n", sp(cg), gv->name` |
| BSS  | `"%s: %s 1\n", gv->name, dir` | `"%s%s: %s 1\n", sp(cg), gv->name, dir` |

Block-scope static label definitions (`sv->label`) already start with `Lstatic_` —
they are internal labels and must **not** receive the prefix.

**Category C — `call` instructions**

| Line | Current | Change |
|------|---------|--------|
| 4076 | `"    call %s\n", node->value` | `"    call %s%s\n", sp(cg), node->value` |
| 4123 | `"    call %s\n", node->value` | `"    call %s%s\n", sp(cg), node->value` |
| 4161 | `"    call %s\n", node->value` | `"    call %s%s\n", sp(cg), node->value` |

**Category D — `[rel gvname]` memory references**

All uses of `gv->name` or `node->value` inside `[rel ...]` expressions are C symbol
names and need the prefix:

| Line | Current | Change |
|------|---------|--------|
| 1244 | `"    lea rax, [rel %s]\n", gv->name` | `"    lea rax, [rel %s%s]\n", sp(cg), gv->name` |
| 1253 | `"    mov rax, [rel %s]\n", gv->name` | `"    mov rax, [rel %s%s]\n", sp(cg), gv->name` |
| 1449 | `"    lea rax, [rel %s + %d]\n", gv->name` | `"    lea rax, [rel %s%s + %d]\n", sp(cg), gv->name` |
| 1451 | `"    lea rax, [rel %s]\n", gv->name` | `"    lea rax, [rel %s%s]\n", sp(cg), gv->name` |
| 1545 | `"    mov rax, [rel %s]\n", gv->name` | `"    mov rax, [rel %s%s]\n", sp(cg), gv->name` |
| 1576 | `"    lea rax, [rel %s]\n", gv->name` | `"    lea rax, [rel %s%s]\n", sp(cg), gv->name` |
| 2626 | `"    lea rax, [rel %s]\n", node->value` | `"    lea rax, [rel %s%s]\n", sp(cg), node->value` |
| 2634 | `"    lea rax, [rel %s]\n", gv->name` | `"    lea rax, [rel %s%s]\n", sp(cg), gv->name` |
| 3249 | `"    lea rax, [rel %s]\n", gv->name` | `"    lea rax, [rel %s%s]\n", sp(cg), gv->name` |

**Category E — block-scope static `dq` symbol references**

In `codegen_emit_local_statics` (around line 8087–8107), several `dq` statements
reference C symbol names on the right-hand side — these need `sp(cg)`:

```
%s: dq %s           → sv->label (Lstatic_ — no prefix) : sv->init_node->value (C name — prefix)
%s: dq %s           → sv->label (Lstatic_ — no prefix) : child->value (C name — prefix)
%s: dq %s           → sv->label (Lstatic_ — no prefix) : arr_name (C name — prefix)
%s: dq %s + %ld     → sv->label (Lstatic_ — no prefix) : arr_name (C name — prefix)
```

**Category F — `.note.GNU-stack` footer**

```c
/* Line 8189 */
if (!cg->target_macho)
    fprintf(cg->output, "\nsection .note.GNU-stack noalloc noexec nowrite progbits\n");
```

---

## Changes to `build.sh`

### Platform detection

Add at the top of `build.sh`, after `SCRIPT_DIR`:

```bash
HOST_OS=$(uname -s)   # Linux or Darwin
HOST_ARCH=$(uname -m) # x86_64 or arm64
```

### Conditional assembler and linker

Replace the hardcoded `nasm -f elf64` and `gcc -no-pie` in `do_bootstrap_build()`:

```bash
if [ "$HOST_OS" = "Darwin" ]; then
    NASM_FMT="macho64"
    LINK_CMD=(clang -arch x86_64)
    COMPILER_TARGET="--target=x86_64-macos"
else
    NASM_FMT="elf64"
    LINK_CMD=(gcc -no-pie)
    COMPILER_TARGET="--target=x86_64-linux"
fi
```

Pass `$COMPILER_TARGET` to the compiler invocation and use `$NASM_FMT` and
`${LINK_CMD[@]}` for the assemble and link steps.

---

## Verification

```bash
# Build the compiler
cmake -S . -B build && cmake --build build

# Compile a test file for x86_64-macos
echo 'int main(void) { return 42; }' > /tmp/t.c
./bin/ccompiler --target=x86_64-macos /tmp/t.c

# Inspect: should see _main: (with underscore), no .note.GNU-stack
grep "_main\|note.GNU" t.asm
# Expected: _main: present, .note.GNU-stack absent

# Assemble and run via Rosetta 2
nasm -f macho64 t.asm -o /tmp/t.o
clang -arch x86_64 /tmp/t.o -o /tmp/t
/tmp/t; echo "exit: $?"
# Expected: exit: 42

# Auto-detect: running on macOS should default to x86_64-macos
./bin/ccompiler /tmp/t.c
# (should produce same _main: output without explicit --target)

# Linux target still works (flag explicitly)
./bin/ccompiler --target=x86_64-linux /tmp/t.c
grep "^main:\|note.GNU" t.asm
# Expected: main: (no underscore), .note.GNU-stack present

rm /tmp/t.c t.asm /tmp/t.o /tmp/t
```

---

## Out of scope

- ARM64 codegen (`--target=arm64-macos` is parsed but exits with
  "not yet implemented") — that is Phase 1+
- Test runner macOS support — that is Phase 0C
- `nasm` installation on macOS — assumed available via Homebrew (`brew install nasm`)
