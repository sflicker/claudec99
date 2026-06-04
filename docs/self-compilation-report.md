# Self-Compilation Diagnostic Report

**Date:** 2026-06-03
**Compiler:** `build/ccompiler` (at commit `9a680c7`)
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | PASS | — |
| `ast_pretty_printer.c` | PASS | — |
| `codegen.c` | PASS | — |
| `compiler.c` | PASS | — |
| `lexer.c` | PASS | — |
| `parser.c` | PASS | — |
| `preprocessor.c` | PASS | — |
| `type.c` | PASS | — |
| `util.c` | PASS | — |
| `version.c` | PASS | — |

**All 10 source modules compiled cleanly.** The compiler successfully
self-compiles its entire `src/` tree with no missing stub headers and no
unimplemented language features encountered. (The previously reported
`lexer.c` failure on hexadecimal integer literals is now resolved — see
commit `4aca002`, "stage 90: add hexadecimal integer literals.")

---

## Category A — Missing stub system headers

None. Every system header required by the source tree is present as a stub
under `test/include/` (`ctype.h`, `errno.h`, `limits.h`, `setjmp.h`,
`stdarg.h`, `stdbool.h`, `stddef.h`, `stdint.h`, `stdio.h`, `stdlib.h`,
`string.h`, `time.h`).

---

## Category B — Language features not yet implemented

None. The compiler parsed and code-generated every module without rejecting
any C99 syntax or semantics used in the source. The historically tracked
gaps — `for`-init declarations (C99 §6.8.5.3), enum/constant-expression
`case` labels (§6.8.4.2), and subscript of member-access expressions
(§6.5.2.1) — are all handled by the current compiler, since the source
itself uses these constructs and compiles successfully.

---

## Successful compilation

All modules compiled cleanly:

- `ast.c`
- `ast_pretty_printer.c`
- `codegen.c`
- `compiler.c`
- `lexer.c`
- `parser.c`
- `preprocessor.c`
- `type.c`
- `util.c`
- `version.c`

Each produced a corresponding `.asm` output and exited with status 0. No
limit overrides (e.g. `MAX_FUNCTIONS`) were required; the default
`include/constants.h` settings were sufficient for every file.

---

## Feature gap summary

| Gap | C99 section | Affected modules |
|---|---|---|
| — | — | None |

No feature gaps observed. The compiler is self-hosting across the full
`src/` source set under the test header stubs.
