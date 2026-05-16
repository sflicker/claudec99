# Stage 50 Kickoff: Object-Like Macros

## Summary of the Spec

Stage 50 adds object-like `#define` macro support to the preprocessor. The preprocessor already handles line splicing, comment stripping, and `#include "file"` expansion. This stage introduces:

- A shared macro table across all recursive include levels (so header-defined macros are visible to the including file)
- Parsing of `#define NAME [replacement]` directives
- Identifier-level macro expansion in ordinary source text
- Duplicate definition checking: same replacement is OK, different replacements are fatal errors

Out of scope: function-like macros, macro arguments, stringification (`#`), token pasting (`##`), conditional preprocessing, recursive macro expansion, and predefined macros.

## Required Tokenizer Changes

None. The tokenizer does not change.

## Required Parser Changes

None. The parser does not change.

## Required AST Changes

None. The AST does not change.

## Required Code Generation Changes

None. Code generation does not change.

## Required Preprocessor Changes

All changes are in `src/preprocessor.c`:

1. **Macro types and table**
   - Add `MacroDef` struct (name + replacement text)
   - Add `MacroTable` struct (dynamic array of `MacroDef`)
   - Add helper functions: `macro_table_init`, `macro_table_free`, `macro_find`, `macro_define`

2. **Duplicate definition handling**
   - `macro_define` checks for duplicate definitions
   - Same replacement = allowed (silent)
   - Different replacement = fatal error containing "incompatible replacement"

3. **Function signature updates**
   - `preprocess_internal` and `preprocess_file` accept a `MacroTable *` parameter
   - Macro table is shared across all recursive include levels

4. **`#define` directive handler**
   - Parse `#define NAME [replacement]` format
   - Reject function-like macros (detect `(` after name)
   - Skip whitespace separator between name and replacement
   - Capture rest of line as replacement text (trailing whitespace trimmed)
   - Store definition in the macro table

5. **Identifier expansion**
   - New path in main preprocessor loop (between directive handling and string-literal handling)
   - When reading identifier (starts with `isalpha(c) || c == '_'`):
     - Read full identifier
     - Look up in macro table
     - Output replacement text if found, or original identifier text if not
   - Replacement text is output verbatim (no re-scanning, no chained expansion)

6. **Entry point update**
   - `preprocess` function creates and frees the `MacroTable`

7. **Header documentation**
   - Update `include/preprocessor.h` doc comment to mention object-like macro substitution

## Test Plan

### Valid tests (`test/valid/`)

1. **`test_pp_define_value__42.c`**
   - `#define VALUE 42`
   - Return VALUE
   - Expected output: 42

2. **`test_pp_define_array__42.c`**
   - `#define SIZE 10`
   - `#define VALUE 42`
   - Array usage with macro constants
   - Return array element
   - Expected output: 42

3. **`test_pp_define_sum_expr__10.c`**
   - `#define SUM 1+2+3+4`
   - Return SUM
   - Expected output: 10

4. **`test_pp_define_compat_redef__42.c`**
   - Compatible redefinition: `#define ANSWER 42` twice
   - Return ANSWER
   - Expected output: 42

5. **`test_pp_define_stmt_macros__2.c`**
   - `#define INIT int a=1;`
   - `#define INC a++;`
   - `#define RETURN return a;`
   - Use macros as statements
   - Expected output: 2

### Integration test (`test/integration/`)

- **`test_pp_define_header/`** directory containing:
  - `constants.h` — defines `#define ANSWER 42`
  - `main.c` — includes `constants.h`, returns `ANSWER`
  - Expected output: 42

### Invalid test (`test/invalid/`)

- **`test_pp_define_incompat_redef__incompatible_replacement.c`**
  - `#define ANSWER 42` followed by `#define ANSWER 43`
  - Error message must contain "incompatible replacement"
