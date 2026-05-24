# Stage 67-01 Kickoff — Opaque FILE and EOF Constant

## Summary

Add `typedef struct FILE FILE;` and `#define EOF (-1)` to the stub `stdio.h` at
`test/include/stdio.h`. This makes the opaque `FILE` type and the `EOF` sentinel
available to programs that `#include <stdio.h>`.

## Required tokenizer changes

None. `FILE` is an ordinary identifier; the typedef keyword and struct tag are
already tokenized.

## Required parser changes

None. `typedef struct FILE FILE;` is already valid syntax — incomplete struct
typedefs have been supported since the struct/typedef stages.

## Required AST changes

None.

## Required code generation changes

None.

## Ambiguity / spec note

The spec names the target directory `ust/include`, which does not exist in the
project. The authoritative stub header location is `test/include/stdio.h`.
Treating the path as a typo.

## Test plan

- Add `test/valid/test_opaque_file_ptr__1.c` — the exact test from the spec:
  declares `FILE *f`, assigns null, and returns `f == 0 && EOF == -1` (expect 1).
- Run the full test suite: all existing tests must continue to pass.
