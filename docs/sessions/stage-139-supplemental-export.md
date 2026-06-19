# Stage 139 Supplemental Session Export

## Key Decisions and System Header Compatibility Strategy

### Problem

After stage 139 shipped three preprocessor `#if` expression fixes, the compiler could parse system headers syntactically but still failed 46 of 98 system header compatibility tests (`run_tests_sysinclude.sh`). These failures fell into eight distinct categories, each representing a different semantic or syntactic gap in the compiler's support for C99 and system header idioms.

### Root Causes and Rationale

The eight compatibility gaps were addressed in order of dependency and severity:

1. **GCC Extension Predefinitions Conflict** — glibc's `sys/cdefs.h` defines `__inline`, `__attribute__`, and `__extension__` as macros, which conflicted with ClaudeC99's builtin preamble. These predefinitions were intended to provide "dummy" definitions for optional GCC extensions, but glibc's explicit definitions took precedence. Decision: remove the conflicting three from the preamble; keep `__restrict__`, `__volatile__`, `__const__` because glibc's `cdefs.h` does not define them.

2. **Block Comment Stripping** — system headers like `math.h` use comment-only macros (`#define __attribute_malloc__ /* Ignore */`). The preprocessor was storing these comments as part of the macro text and emitting them as tokens in the output, causing "unexpected /" errors. Decision: add a `strip_block_comments()` helper to remove `/* ... */` sequences from macro replacement text before storage.

3. **Function-Type Typedefs** — system headers (e.g., `cookie_io_functions_t.h`) use function-type typedefs: `typedef ssize_t cookie_read_function_t(void *, char *, size_t);`. ClaudeC99's parser only supported scalar, pointer, and array typedefs. Decision: extend `parse_type_specifier` to recognize function prototypes following a typename, parse the parameter list via paren-balanced skip, and register the name as a placeholder type so struct members can reference it.

4. **`long double` Type Specifier** — `bits/floatn-common.h` uses `typedef long double _Float64x;`. The parser rejected the `long double` combination. Decision: add a check in `parse_type_specifier` for `TOKEN_DOUBLE` after `TOKEN_LONG` and treat it as `TYPE_DOUBLE`.

5. **Unnamed Array Parameters** — function prototypes use unnamed array dimensions: `extern char *tmpnam(char[L_tmpnam]);`. The parser expected an identifier before the `[`. Decision: add an early-return path in `parse_parameter_declaration` that detects `TOKEN_LBRACKET` without a preceding identifier, skips dimensions via bracket-balanced loop, and converts the parameter type from array to pointer.

6. **Cast Expressions in Constant Expressions** — `#define __NFDBITS (8 * (int) sizeof(__fd_mask))` contains a cast in a constant expression. The preprocessor evaluator only handled grouped expressions. Decision: modify `eval_const_primary` to distinguish `(expr)` from `(type)expr` by checking for type keywords after the opening paren, then parse the type, consume the closing paren, and evaluate the cast operand.

7. **Preamble Injection Control** — the `__builtin_va_list` preamble definition was injected into `--print-tokens` mode output, breaking ~150 expected files. Decision: add an `inject_preamble` parameter to `preprocess_with_defines_and_includes` and set it to 0 when `--print-tokens` is active.

8. **Self-Host String Concatenation** — ClaudeC99 does not yet support adjacent string literal concatenation. The builtin preamble string was split across multiple lines relying on this feature, preventing the compiler from self-compiling. Decision: collapse the preamble into a single long string.

### Compatibility Improvement

- **Before**: 52/98 system header tests pass (53%)
- **After**: 98/98 system header tests pass (100%)
- **Tests added**: 1 integration test (`test_long_unsigned_ordering`, committed this session)
- **Regular test suite**: all 1979 tests pass; no regressions

### Self-Hosting Impact

All changes are localized to preprocessor, parser, and codegen paths that are either dormant or exercised during normal compilation. The self-host bootstrap (C0→C1→C2) verified successfully with all 1979 tests passing at every stage; no source modifications required.

### Design Philosophy

These fixes prioritize pragmatic header compatibility without introducing complex new language features. Each gap is filled with minimal, targeted changes:
- Removal of redundant predefinitions rather than adding override logic
- Comment stripping applied consistently across all macro types
- Function-type typedef registration via placeholder (no full type representation)
- Early-return paths for edge cases rather than complex parser state machines
- Conditional injection of preamble rather than post-processing

The strategy avoids planned-future features (K&R function declarations, complex function pointer types) and focuses on the specific idioms found in system headers.
