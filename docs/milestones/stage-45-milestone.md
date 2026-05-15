# Milestone Summary

## stage-45: Basic libc declarations and external calls - Complete

stage-45 ships libc function prototyping and integration testing, enabling calls to external functions including void* parameters/returns and const char* parameters.

- Tokenizer: None.
- Grammar/Parser: None.
- AST/Semantics: None.
- Codegen: Replaced strict `pointer_types_equal` with `pointer_types_assignable` at function-call argument-passing sites, allowing void* to interoperate with object pointers (matching existing assignment/return rules).
- Tests: Added test/integration/ harness with companion-file support (.expected, .libs, .args, .input, .status). Migrated 7 existing valid tests to integration suite. Added 4 new integration tests (puts with size_t, strcmp, strlen, malloc/free). All 11 integration tests pass.
- Docs: Updated README.md to reference stage 45, mention integration suite and libc prototype support, and reflect new test totals (885 aggregate, up from 881).
- Notes: Spec contained typos in companion-file naming (LFLAGS_FILE vs .libs, .exticode vs .status) and sample-code defects (missing } before else in strcmp, unclosed string in strlen). Resolved per table definitions and fixed samples during transcription. The void* compatibility gap was implicitly required by the malloc/free test and addressed as part of codegen.
