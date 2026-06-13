# stage-107 Transcript: `inline` Keyword, `<assert.h>`, and `va_copy` Codegen

## Summary

Stage 107 closes three independent C99 gaps. First, the `inline` function specifier is tokenized and consumed in declaration specifiers, allowing declarations like `inline int f(void)` and `static inline void g(void)` to parse successfully (previously a hard error). Second, `test/include/assert.h` is created with a complete NDEBUG-aware `assert` macro that leverages existing preprocessor features (stringification, predefined macros, and `fprintf`/`abort`). Third, `va_copy` codegen is corrected to emit three 8-byte moves (rax intermediate) that copy the 24-byte SysV AMD64 `va_list` struct from source to destination on the stack, replacing a silent no-op stub. A preprocessor bug discovered during assert macro testing — `__FILE__` and `__LINE__` not expanding inside function-like macro bodies — is fixed via static globals tracking the current file and line during rescan. All three features are self-contained with no new AST nodes or grammar changes.

## Changes Made

### Step 1: Tokenizer — `inline` Keyword

- Added `TOKEN_INLINE` to the token type enum in `include/token.h`.
- Updated `src/lexer.c` keyword recognition branch to match `"inline"` and set `token.type = TOKEN_INLINE`.
- Added `TOKEN_INLINE` case to `token_type_name()` display table returning `"'inline'"`.

### Step 2: Parser — `inline` Declaration Specifier Consumption

- Updated `parse_declaration_specifiers` in `src/parser.c` with a new branch consuming `TOKEN_INLINE` and discarding it (parse-and-ignore, matching `restrict` and `volatile` patterns).
- Branch positioned after `TOKEN_VOLATILE` and before `TOKEN_EXTERN`/`TOKEN_STATIC`/`TOKEN_TYPEDEF`.
- No other parser changes required; `inline` does not appear inside declarators.

### Step 3: Preprocessor Bug Fix — `__FILE__` and `__LINE__` Expansion in Macros

- Added static globals `g_expand_source_path` and `g_expand_current_line` to `src/preprocessor.c` to track the current file and line during macro expansion and rescan.
- Updated `expand_macros_text` to set these globals before rescanning and to substitute `__FILE__` → lexer-stored path string and `__LINE__` → decimal line number during `expand_macros_text` rescan.
- Bug: `__FILE__` and `__LINE__` predefined macros were not expanding inside function-like macro bodies during the rescan phase (they were already expanded at definition parse time but stripped on rescan). Fix ensures they expand to current values at each rescan point.

### Step 4: Header — `<assert.h>` Stub

- Created `test/include/assert.h` with two-branch implementation: `NDEBUG` defined → `assert(expr)` expands to `((void)0)` (no-op); otherwise → comma expression using `fprintf(stderr, ...)`, `#expr` stringification, `__FILE__`, `__LINE__`, and `abort()`.
- Header includes `<stdio.h>` and `<stdlib.h>` (guarded by their own `#ifndef` include guards).
- Matches C99 §7.2 semantics: with NDEBUG, assertion expressions are not evaluated and expand to a void expression.

### Step 5: Code Generation — `va_copy` 24-Byte Struct Copy

- Located the combined no-op branch handling `AST_BUILTIN_VA_END` and `AST_BUILTIN_VA_COPY` in `src/codegen.c` (previously around line 3971).
- Split into two separate blocks: `va_end` remains a no-op (set `result_type = TYPE_VOID` and return); `va_copy` now emits code.
- `va_copy` codegen: reads operands (dst and src), looks up local variable offsets via `codegen_find_var`, emits three 8-byte `mov rax` → `mov [dst]` pairs copying words at offsets 0, -8, and -16 bytes relative to rbp.
- Uses rax as a temporary (safe because `va_copy` result is void and never appears in a value context).

### Step 6: Version Bump

- Updated `VERSION_STAGE` in `src/version.c` from `"01060000"` to `"01070000"`.

### Step 7: Tests

- Added 8 new valid tests:
  - `test_inline_func__0.c` — parses and executes `inline int square(int x)`.
  - `test_static_inline__0.c` — parses and executes `static inline int add(int, int)`.
  - `test_extern_inline__0.c` — parses extern and inline declarations.
  - `test_assert_pass__0.c` — asserts that pass and return 0.
  - `test_assert_ndebug__0.c` — asserts with NDEBUG defined (no-op, returns 0).
  - `test_assert_fail__134.c` — assert(0) aborts with signal SIGABRT (exit 128+6=134).
  - `test_va_copy_basic__0.c` — va_copy creates independent copy of va_list; two iterations produce same sum.
  - `test_void_implicit_return__0.c` — void function falls off end without explicit return (regression test).

## Final Results

All 1615 tests pass (930 valid, 255 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). No regressions. Full self-host C0→C1→C2 cycle passes cleanly with all tests green at every stage.

## Session Flow

1. Read specification and supporting files (test/include headers, stage 106 checklist).
2. Reviewed existing `inline` parsing gap and va_copy stub at codegen.c:3971.
3. Identified preprocessor bug with `__FILE__`/`__LINE__` inside function-like macros during assert.h design.
4. Implemented tokenizer and parser changes for `inline` keyword (parallel to `restrict` stage 106).
5. Fixed preprocessor expansion with static globals for file/line context tracking.
6. Created assert.h stub with NDEBUG guarding and macro stringification.
7. Split and implemented va_copy 24-byte struct copy codegen (three 8-byte moves via rax).
8. Added version bump to stage 107.
9. Created 8 new test cases covering inline, assert, va_copy, and implicit void return.
10. Built and ran full test suite; all tests pass.
11. Ran self-host cycle C0→C1→C2; all stages pass with 1615 tests each.
12. Generated milestone and transcript documentation.
