# Stage 48 Kickoff - Preprocessor Foundation

## Summary of the spec

This stage introduces a standalone preprocessing pass that runs before the tokenizer, establishing a new compilation pipeline:

```
source file → preprocessor → tokenizer → parser → codegen
```

The preprocessor handles three main transformations:

1. **Line splicing**: Remove backslash-newline sequences to join logical lines
2. **Comment stripping**: Replace line (`//`) and block (`/* */`) comments with a single space
3. **Directive detection**: Detect preprocessor directives (lines starting with `#`) and reject them with "unsupported directive" errors

Comments are currently handled by the lexer. This stage moves comment handling to the preprocessor and removes it from `lexer_skip_whitespace()`.

Out of scope: macros, `#include`, `#ifdef`, `#define`, macro expansion, and all other preprocessor features except comment/whitespace handling and directive error reporting.

## Required tokenizer changes

Remove comment handling logic from `lexer_skip_whitespace()`. The function should skip only whitespace characters (space, tab, newline, carriage return, form feed). Comment removal is now the preprocessor's responsibility.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation changes

None.

## Test plan

### Valid tests (should compile and run successfully)

- `test_pp_line_comment`: Line comment (`//`) at start of file
- `test_pp_eol_comment`: End-of-line comment after code
- `test_pp_block_comment`: Block comment (`/* */`) in expression
- `test_pp_multiline_block_comment`: Multi-line block comment
- `test_pp_comment_between_tokens`: Comment between tokens (e.g., `4/*comment*/+2`)
- `test_pp_line_splice`: Line splicing with backslash-newline (e.g., `42\n` → `42`)
- `test_pp_line_splice_ident`: Line splicing across identifier boundaries

### Invalid tests (should produce compiler errors)

- `test_pp_comment_join_tokens`: Comment between two number tokens (`4/*comment*/2`) — parser error, not preprocessor
- `test_pp_unsupported_define`: `#define` directive — should error with "unsupported directive"
- `test_pp_unsupported_include`: `#include` directive — should error with "unsupported directive"
- `test_pp_unterminated_block_comment`: Unclosed `/*` comment — should error

## Implementation notes

- Add `include/preprocessor.h` and `src/preprocessor.c`
- Update `compiler.c` to call `preprocess()` before `lexer_init()`
- Update `CMakeLists.txt` to compile `src/preprocessor.c`
- Comments must be replaced with a single space, not removed entirely (to preserve token separation)
- Line splicing occurs before comment removal and tokenization
