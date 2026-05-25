# Stage 70.03 – Add Line/Column to Errors and Warnings

## Summary of the spec

Add `<file>:<line>:<col>:` prefix to all compiler error and warning messages, using the line/column information already stored in Token structs from stage 70.02. Transform error messages like:

```
error: cannot declare variable 'x' of type void
```

Into messages like:

```
test_void_var_decl__cannot_declare.c:2:10: error: cannot declare variable 'x' of type void
```

Apply the same format to warning messages.

## Required tokenizer changes

None. Token structs already store line and column information from stage 70.02.

## Required parser changes

The parser is the primary source of errors (error messages are issued when tokens are analyzed). We will:

1. Add a static helper `parser_error(Parser *parser, const char *fmt, ...)` that extracts the current token's position and calls the new `compile_error_at()` function.
2. Replace approximately 106 `compile_error()` calls with `parser_error(parser, ...)` calls.

## Required AST changes

None. AST nodes do not need position fields for this stage (only tokens carry position info).

## Required code generation changes

The only existing warning in the codebase is `codegen_warn_const_discard` in `src/codegen.c`. Since the code generator walks AST nodes (not tokens), we will:

1. Add a new helper `compile_warning_at()` (matching the signature of `compile_error_at()`).
2. Update `codegen_warn_const_discard` to call `compile_warning_at(NULL, 0, 0, ...)` with NULL file and 0/0 for line/col.
3. This ensures consistent `-Werror` behavior with the new position format.

## Test plan

1. Run the existing `invalid` test suite to verify error message format changes.
2. The test harness already uses substring matching (`grep -qi` for invalid tests), so no harness changes are needed.
3. Integration error file matching already uses `grep -qF`, so test updates are not required.
4. Verify that `-Werror` flag still promotes warnings to errors correctly.

## Implementation order

1. `include/util.h` – Add function declarations for `compile_error_at()` and `compile_warning_at()`
2. `src/util.c` – Implement both functions with `file:line:col:` prefix formatting
3. `src/parser.c` – Add `parser_error()` helper and replace ~106 `compile_error()` calls
4. `src/codegen.c` – Update `codegen_warn_const_discard` to use `compile_warning_at(NULL, 0, 0, ...)`
5. Build and verify no build errors
6. Run test suite and validate error/warning message format

## Known ambiguities and decisions

1. **Grammar typo correction**: The spec contains "Update it some it would be" which should be "Update it so it would be." This is a documentation issue, not a code issue.

2. **NULL file and zero position for warnings**: Since the code generator currently has no position information, we pass `NULL` for file and `0` for line/col to `compile_warning_at()`. If the file is NULL or line is 0, we emit the warning without the position prefix (maintaining backward compatibility for codegen warnings).

3. **No grammar.md changes**: This stage makes no changes to the language grammar or AST structure.

4. **No test harness changes**: Existing substring matching in the test harness is sufficient for validating the new format.
