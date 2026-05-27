# stage-75-02 Transcript: stdarg.h and va_list Surface

## Summary

This stage introduces surface-level support for variadic function argument access by defining `stdarg.h` with the `va_list` typedef and four va_* macro stubs. The `va_list` type is defined as an array-of-one struct (matching the System V AMD64 ABI calling convention), containing fields for general-purpose offset, floating-point offset, overflow argument area pointer, and register save area pointer. The va_* macros (`va_start`, `va_end`, `va_copy`, `va_arg`) expand to corresponding `__builtin_*` intrinsics that do not yet exist; using these macros is out of scope for this stage. Additionally, the parser is fixed to implement proper C99 array-to-pointer decay for function parameters declared with array-typedef types.

## Changes Made

### Step 1: Parser — Array-to-Pointer Parameter Decay

- Modified `parse_parameter_declaration()` in `src/parser.c` to detect when a parameter type is an array typedef.
- When a `TYPE_ARRAY` is encountered in a parameter context, the parser now creates a `TYPE_POINTER` wrapping the array's element type, implementing C99 §6.7.5.3p7 decay semantics.
- Full-type chain (pointer-to-element) is properly constructed so that `sizeof()` and parameter passing work correctly on the decayed pointer type.

### Step 2: stdarg.h Header

- Created `test/include/stdarg.h` with the following contents:
  - `#ifndef` / `#define` header guard using `CLAUDEC99_STDARG_H`.
  - Typedef for `va_list` as `struct __claudec00_va_list_tag [1]`, containing four fields: `unsigned int gp_offset`, `unsigned int fp_offset`, `void *overflow_arg_area`, `void *reg_save_area`.
  - Four macro definitions: `va_start(ap, last)` → `__builtin_va_start(ap, last)`, `va_end(ap)` → `__builtin_va_end(ap)`, `va_copy(dst, src)` → `__builtin_va_copy(dst, src)`, `va_arg(ap, type)` → `__builtin_va_arg(ap, type)`.

### Step 3: Version Update

- Updated `src/version.c` to set `VERSION_STAGE` from `"00750100"` to `"00750200"`.

### Step 4: Tests — va_list Type and Parameter Passing

- Added `test/valid/test_va_list_sizeof__1.c`: declares `va_list ap`, returns `sizeof(ap) > 0` (expected exit code 1).
- Added `test/valid/test_va_list_param__1.c`: defines a helper function accepting `va_list ap`, compares it to null pointer (expected exit code 1); main function declares `va_list ap` and passes it to the helper.

### Step 5: Build and Test

- Built the compiler without errors.
- Ran full test suite: 1184 tests pass (previously 1182); 2 new valid tests added, 0 failures, no regressions.

## Final Results

All 1184 tests pass (739 valid, 219 invalid, 67 integration, 39 print-AST, 99 print-tokens, 21 print-asm). The stage is complete with no regressions.

## Session Flow

1. Read stage 75-02 spec and supporting template files.
2. Reviewed spec requirements and identified needed changes (parser fix, stdarg.h file, test cases, version update).
3. Noted and corrected two spec typos (`__buildin_va_start` → `__builtin_va_start`, `stdard.h` → `stdarg.h`).
4. Implemented array-to-pointer decay in `parse_parameter_declaration()` to handle array-typedef function parameters.
5. Created `test/include/stdarg.h` with va_list typedef and macro stubs matching SysV AMD64 ABI.
6. Updated version string in `src/version.c`.
7. Added two new valid test cases for va_list declaration and parameter passing.
8. Built compiler and ran full test suite; verified all 1184 tests pass with no regressions.
9. Generated milestone summary, transcript, and updated README.md and docs/grammar.md.

## Notes

- The spec contained a typo in va_start macro definition (`__buildin_va_start` instead of the correct `__builtin_va_start`); the implementation uses the correct intrinsic name.
- The spec contained a header filename typo (`stdard.h` instead of `stdarg.h`); the implementation uses the correct filename.
- Array-to-pointer decay for function parameters was implicitly required to make test_va_list_param__1.c work (passing va_list by value to a function), but was not explicitly stated in the spec; the implementation follows C99 standard semantics.
- The `__builtin_*` intrinsics referenced by the va_* macros do not yet exist in the compiler; attempting to use these macros would fail at compilation. A future stage will implement the intrinsics.
