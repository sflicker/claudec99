# ClaudeC99 Stage 178 — macOS ARM64 Build (Phase 0A)

## Goal

Make the compiler binary build and run cleanly as a native ARM64 executable on
macOS (Apple M-series). No new language features; no backend changes. The output
assembly remains x86-64 Linux NASM — the target of this stage is the **compiler
binary itself**, not the code it generates.

---

## Background

A probe build on the M1 (`cmake -S . -B build/arm64_probe && cmake --build build/arm64_probe`)
already produces a working native `Mach-O 64-bit executable arm64` binary. The
compiler binary runs and generates correct x86-64 NASM assembly. However, Apple
Clang raises three categories of warnings that GCC (the CI/Linux toolchain) does
not emit. All must be fixed so the build is clean under `-Wall -Wextra -pedantic`
on both toolchains.

---

## Findings

### Warning 1 — `src/compiler.c`: incomplete switch over `TokenType` (`-Wswitch`)

`token_type_name()` has a `switch (TokenType)` with no `default:` case. Apple Clang
(unlike GCC) warns on every enum value not covered by a case. Nine values added in
later stages were never added to this diagnostic-only function:

```
TOKEN_UNION, TOKEN_FLOAT, TOKEN_DOUBLE, TOKEN_RESTRICT, TOKEN_INLINE,
TOKEN_AUTO, TOKEN_REGISTER, TOKEN_FLOAT_LITERAL, TOKEN_DOUBLE_LITERAL
```

**Fix:** Add the nine missing cases.

### Warning 2 — `src/parser.c:3355`: `/*` within block comment (`-Wcomment`)

The comment at line 3355 reads:

```c
 * Supports: FP/int literals, unary +/-, binary +/-/* and /, parentheses.
```

The sequence `/*` inside a `/* ... */` comment triggers `-Wcomment`.

**Fix:** Rephrase to avoid `/*` — e.g. change `+/-/* and /` to `+/-/*/÷`.

### Warning 3 — `src/type.c`: partial struct initializers (`-Wmissing-field-initializers`)

The 14 static singleton `Type` values are positionally initialized with 6 of the
struct's 11+ fields. The remaining fields are zero per C99 §6.7.9p21, so the code
is correct. GCC does not warn; Apple Clang emits one warning per singleton.

Switching all 14 initializers to fully explicit form would be very verbose and add
no semantic value. **Fix:** Suppress `-Wmissing-field-initializers` for Apple Clang
only via a compiler-id guard in `CMakeLists.txt`. This is the appropriate response
when the code is correct C99 and the warning is toolchain-specific.

---

## Changes

### `src/compiler.c`

Add nine missing cases to `token_type_name()` after the existing `TOKEN_STRUCT` case:

```c
case TOKEN_UNION:          return "TOKEN_UNION";
case TOKEN_FLOAT:          return "TOKEN_FLOAT";
case TOKEN_DOUBLE:         return "TOKEN_DOUBLE";
case TOKEN_RESTRICT:       return "TOKEN_RESTRICT";
case TOKEN_INLINE:         return "TOKEN_INLINE";
case TOKEN_AUTO:           return "TOKEN_AUTO";
case TOKEN_REGISTER:       return "TOKEN_REGISTER";
case TOKEN_FLOAT_LITERAL:  return "TOKEN_FLOAT_LITERAL";
case TOKEN_DOUBLE_LITERAL: return "TOKEN_DOUBLE_LITERAL";
```

### `src/parser.c`

Line 3355 — rephrase the offending comment:

```c
 * Supports: FP/int literals, unary +/-, binary +/-/div and /, parentheses.
```

### `CMakeLists.txt`

After the existing `-Wall -Wextra -pedantic` options, add an Apple Clang guard for
both targets:

```cmake
if(CMAKE_C_COMPILER_ID MATCHES "AppleClang")
    target_compile_options(ccompiler PRIVATE -Wno-missing-field-initializers)
    target_compile_options(casm      PRIVATE -Wno-missing-field-initializers)
endif()
```

---

## Verification

After the changes:

```bash
# Clean build in the standard build directory
cmake -S . -B build
cmake --build build 2>&1 | grep -E "warning:|error:"
# Expected: no output (zero warnings, zero errors)

# Binary is native ARM64
file bin/ccompiler
# Expected: bin/ccompiler: Mach-O 64-bit executable arm64

# Compiler runs and produces output
echo 'int main(void) { return 42; }' > /tmp/t.c
./bin/ccompiler /tmp/t.c
cat t.asm
# Expected: valid x86-64 NASM assembly
rm /tmp/t.c t.asm

# Front-end test suites (no assembler needed)
test/print_tokens/run_tests.sh
test/print_ast/run_tests.sh
test/invalid/run_tests.sh
# Expected: all pass
```

---

## Out of scope

- No changes to the assembler/linker invocation (that is Phase 0B)
- No ARM64 codegen (that is Phase 1+)
- No test runner changes for macOS (that is Phase 0C)
