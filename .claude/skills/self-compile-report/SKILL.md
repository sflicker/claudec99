---
name: self-compile-report
description: Compiles each source module in src/ one at a time using the project's own built compiler, then analyses every error and produces docs/self-compilation-report.md classifying each failure as a missing stub header or an unimplemented language feature.
---

## Goal

Use the project's own built compiler to attempt to compile each `.c` file in
`src/` independently.  Collect all errors, diagnose their root causes, and
write a structured report to `docs/self-compilation-report.md`.

## Setup

Locate the compiler and set include flags:

```
COMPILER = <project-root>/build/ccompiler
IFLAGS   = -I<project-root>/include -I<project-root>/test/include
```

If `build/ccompiler` does not exist or is not executable, stop and tell the
user to build first with `cmake --build build`.

## Compilation pass

Compile every file matching `src/*.c` one at a time, in alphabetical order,
using:

```
ccompiler --max-errors=0 <IFLAGS> src/<module>.c
```

Capture stdout + stderr for each file.  Record the exit code (0 = success,
non-zero = failure).

## Limits 
If the limits are ran into like MAX_FUNCTIONS try overriding the default settings
in <project-root>/include/constants.h using -D options if possible too increase the 
limit. Don't change the values in constants.h but if it works with the -D options
make a note in the report what values worked.

## Analysis

For each file that fails, identify the **root cause** of the first (or
dominant) error before cataloguing the cascade.  Classify every distinct root
cause into one of these categories:

### Category A — Missing stub system header

The error is:

```
error: include file not found: <header.h>
```

The file needs a system header that has no stub in `test/include/`.  This is
**not** a compiler bug — the stub simply hasn't been written yet.

Note which files are affected transitively (e.g. a missing header in a shared
`.h` file will block every `.c` that includes it).

### Category B — Unimplemented language feature

The compiler parsed past the includes but rejected valid C99 syntax or
semantics.  Common patterns to watch for:

- **`for`-init declarations** — `for (int i = 0; …)` triggers
  `error: expected expression, got 'int'`.  C99 §6.8.5.3.
- **Enum/constant-expression `case` labels** — `case ENUM_VALUE:` triggers
  `error: expected integer literal, got identifier`.  C99 §6.8.4.2.
- **Subscript of a member-access expression** — `ptr->array[i]` triggers
  `error: subscript base must be an identifier`.  C99 §6.5.2.1.
- Any other parser or codegen rejection of otherwise valid C99.

Note that a single root-cause error often produces many cascade errors in
subsequent lines of the same function.  Count and quote only the root cause;
mention that cascades follow.

## Report format

Write (or overwrite) `docs/self-compilation-report.md` using the structure
below.  Include today's date, the compiler path, and the flags used.

```markdown
# Self-Compilation Diagnostic Report

**Date:** <YYYY-MM-DD>
**Compiler:** `build/ccompiler`
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | FAIL/PASS | … |
…

---

## Category A — Missing stub system headers

… one subsection per missing header …

---

## Category B — Language features not yet implemented

… one subsection per distinct feature gap, with a table of affected
file:line locations …

---

## Successful compilation

… list any modules that compiled cleanly, and note why they succeeded …

---

## Feature gap summary

| Gap | C99 section | Affected modules |
|---|---|---|
…
```

## Output

After writing the report, print a one-line summary to the user:

```
self-compile-report: wrote docs/self-compilation-report.md
  <N> modules passed, <M> failed
  Category A: <count> missing stub headers
  Category B: <count> unimplemented language features
```
