# Milestone Summary

## Stage 43: File-scope array and string initializers - Complete

stage-43 ships file-scope initialization for arrays and string-literal pointer assignments.

- Tokenizer: No changes.
- Grammar/Parser: Extended array declaration parsing to support string-literal and brace-list initializers at file scope. Added size inference from initializer lists for both block-scope and file-scope arrays (previously only block-scope char arrays from strings could omit size).
- AST/Semantics: Added `init_node` field to `GlobalVar` struct in `codegen.h` to carry initializer AST nodes to code generation.
- Codegen: Implemented four new initializer emission paths: (1) char array from string literal (`.data` with db bytes), (2) char pointer from string literal (`dq` address), (3) array of char pointers from string-literal list (`dq` addresses), (4) integer array from constant-value list (`dq` values).
- Tests: Four new tests added covering all four cases. Full suite 538/538 pass (up from 534).
- Docs: Updated `docs/grammar.md` and `README.md` to reflect new file-scope initializer support.
- Notes: Size inference enabled for any array with brace-list or string-literal initializer; previously only block-scope char arrays from strings could use `[]`.
