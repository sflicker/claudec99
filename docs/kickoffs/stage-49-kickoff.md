# Stage 49 Kickoff: Quoted Local Includes

## Spec Summary

Stage 49 extends the preprocessor to support local quoted `#include "file.h"` directives. The preprocessor will search for included files relative to the directory containing the including file. The stage preserves all Stage 48 behaviors: line splicing, comment removal, and unsupported directive rejection. Recursive includes are detected via a maximum depth limit (64), and missing include files trigger compilation errors.

## Required Changes

### Tokenizer
None.

### Parser
None.

### AST
None.

### Code Generation
None.

### Preprocessor
**include/preprocessor.h:**
- Change signature from `char *preprocess(const char *source)` to `char *preprocess(const char *source, const char *source_path)`
- The `source_path` parameter enables resolving includes relative to the containing file's directory

**src/preprocessor.c:**
- Full rework of the preprocessing logic:
  - Implement growable output buffer for handling dynamic content
  - Recognize and parse `#include "filename"` directives
  - Recursively preprocess included files using `source_path` to locate them
  - Track include depth (max 64) to detect recursive includes
  - Report missing include files as fatal errors
  - Continue to support line splicing and comment removal
  - Reject unsupported directives (e.g., `#include <...>`, `#define`, `#ifdef`)

**src/compiler.c:**
- Update the call to `preprocess()` to pass `source_file` as the second argument

### Test Harness
**test/integration/run_tests.sh:**
- Add support for `.error` files: if a test has an accompanying `<basename>.error` file, the test harness should verify that compilation fails with an error message

## Test Plan

Four integration test directories will be added:

1. **test_pp_include_local**: Basic local include
   - Files: add.h (function declaration), add.c (includes add.h, function implementation), main.c (includes add.h, calls add)
   - Behavior: Link add.c and main.c, compile and link successfully
   - Expected result: Exit code 7 (return value of add(3, 4))

2. **test_pp_include_nested**: Nested includes
   - Files: multi.h (function declaration), helper.h (includes multi.h), multi.c (includes multi.h, function implementation), main.c (includes helper.h, calls multi)
   - Behavior: Includes are transitively resolved through helper.h
   - Expected result: Exit code 12 (return value of multi(3, 4))

3. **test_pp_include_recursive**: Recursive include detection
   - Files: helper.h (includes itself), main.c (includes helper.h)
   - Behavior: Compilation should fail when recursive include is detected (depth limit exceeded)
   - Expected result: Compilation error (uses .error file)

4. **test_pp_include_missing**: Missing include file
   - Files: main.c (includes nonexistent file)
   - Behavior: Compilation should fail when required file cannot be found
   - Expected result: Compilation error (uses .error file)

## Ambiguities and Decisions

1. **Spec typo**: The opening example shows `#include 'helper.h"` (mismatched quotes). Implementation follows the description and supports only the double-quoted form `#include "filename"`.

2. **Stray semicolon**: The spec shows `#include "add.h";` with a trailing semicolon. The preprocessor will consume the semicolon as trailing content on the directive line (ignored, non-fatal).

3. **Search path**: Include files are searched only relative to the including file's directory. No `-I` path support or system header paths are implemented (out of scope).

4. **Maximum depth**: Recursive includes are prevented via a hardcoded maximum depth of 64. This is checked during recursive preprocessing.

5. **File path construction**: Directory separation uses `/` (standard on POSIX systems); the implementation must handle both absolute and relative file paths correctly.

## Implementation Order

1. Update `include/preprocessor.h` with new signature
2. Implement `src/preprocessor.c` with include handling
3. Update `src/compiler.c` to pass source file path
4. Update `test/integration/run_tests.sh` to support `.error` files
5. Create four integration test directories

## Notable Constraints

- The preprocessor now needs filesystem access to read included files
- Output buffer must grow dynamically as includes expand the source
- Error messages must be clear (missing files, recursive includes)
- Depth tracking must be thread-safe if concurrency is added in future stages
