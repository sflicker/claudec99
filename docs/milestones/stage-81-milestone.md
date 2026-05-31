# Milestone Summary

## stage-81: Header Updates and Compiler Limits - Complete

stage-81 adds two stub library functions (putchar, calloc) and removes an incorrect operator restriction that blocked `!` on pointer operands.

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- Codegen: Removed the restriction that rejected `!` (logical-not) on pointer operands; pointers now use the same negation pattern as 64-bit integers (`cmp rax, 0; sete al`), valid per C99 scalar-type semantics.
- Headers: Added `int putchar(int);` to stdio.h stub; added `void *calloc(size_t nmemb, size_t size);` to stdlib.h stub.
- Constants: Raised four limits in include/constants.h from 64 to 256: PARSER_MAX_FUNCTIONS, PARSER_MAX_GLOBALS, MAX_GLOBALS, MAX_LOCALS.
- Tests: 4 valid tests (calloc allocation and zero-init, putchar output, combined usage, null-pointer negation); 2 invalid tests removed (previously tested invalid `!` on pointers). Full suite 1246/1246 pass.
- Docs: README.md updated (stage reference, supported functions, compiler limits table, test totals).
- Notes: Pointer negation treats pointers as scalars per C99, enabling the common idiom `if (!p)` to check for NULL.
