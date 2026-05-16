# Stage 52-01 Kickoff: Basic Conditional Processing

## Summary

Add support for basic conditional preprocessing directives: `#ifdef`, `#ifndef`, `#else`, and `#endif`. These directives control whether code regions are included or excluded during preprocessing based on macro definitions.

## Spec Reference

[ClaudeC99-spec-stage-52-01-basic-conditional-processing.md](../stages/ClaudeC99-spec-stage-52-01-basic-conditional-processing.md)

## Required Changes by Subsystem

### Tokenizer
No changes required.

### Parser
No changes required.

### AST
No changes required.

### Code Generation
No changes required.

### Preprocessor (src/preprocessor.c and include/preprocessor.h)
All changes are localized to the preprocessor:

**Data Structures:**
- Add `CondFrame` struct to track conditional state (file scope)
- Add `MAX_COND_DEPTH` constant (64) for nesting limit
- Maintain `cond_stack[MAX_COND_DEPTH]` array in `preprocess_internal()`
- Track `cond_depth` and `emitting` state during preprocessing

**Directive Handling:**
- `#ifdef NAME`: Push frame; check if NAME is defined; set active flag
- `#ifndef NAME`: Push frame; check if NAME is NOT defined; set active flag
- `#else`: Toggle active flag; error if duplicate
- `#endif`: Pop frame; error if depth is 0

**Emission Logic:**
- Skip all directives except conditionals when in inactive region
- Skip `#define`, `#include`, and regular content emission when inactive
- Always emit newlines to preserve line structure
- Process `#define` normally when in active region

**Error Detection:**
- After main preprocessing loop: error if `cond_depth > 0` (missing `#endif`)
- Error on `#else` or `#endif` without matching `#ifdef`/`#ifndef`
- Error on duplicate `#else` in same conditional block

**Documentation:**
- Update `include/preprocessor.h` doc comment to document conditional behavior

## Test Plan

### Valid Tests (test/valid/)
1. `test_pp_ifdef_true_branch__42.c` — Active `#ifdef` block with macro defined
2. `test_pp_ifdef_false_branch__1.c` — Inactive `#ifdef` block with macro undefined
3. `test_pp_ifndef_true_branch__42.c` — Active `#ifndef` block with macro undefined
4. `test_pp_ifdef_macro_in_active_block__42.c` — `#define` inside active conditional
5. `test_pp_ifdef_skip_invalid_source__42.c` — Invalid source in inactive region skipped

### Invalid Tests (test/invalid/)
1. `test_pp_ifdef_missing_endif__missing_endif.c` — Missing `#endif`
2. `test_pp_else_without_conditional__else_without_conditional.c` — `#else` with no matching `#ifdef`/`#ifndef`
3. `test_pp_endif_without_conditional__endif_without_conditional.c` — `#endif` with no matching conditional
4. `test_pp_ifdef_duplicate_else__duplicate_else.c` — Duplicate `#else` in same block

## Implementation Order

1. Define `CondFrame` struct and `MAX_COND_DEPTH` constant
2. Add conditional state locals to `preprocess_internal()`
3. Implement `#ifdef` and `#ifndef` handling (push/check/set active)
4. Implement `#else` handling (toggle active, detect duplicates)
5. Implement `#endif` handling (pop frame)
6. Add inactive region skipping logic for all non-conditional directives and content
7. Ensure newlines always emitted
8. Add final error check for missing `#endif`
9. Update documentation in `include/preprocessor.h`
10. Create and run test cases

## Spec Issues Noted

1. **Line 59**: Stray `**` (incomplete/truncated bullet point)
2. **Line 111**: Include guard example missing `#` before `ifndef`
3. **Lines 148-154**: False branch test case missing `#endif`
4. **Lines 88-90**: Illustration shows `{` instead of `}` (typo, not affecting tests)

## Key Decisions

- Stack-based nesting management for conditional blocks
- Depth limit of 64 to prevent stack overflow
- Always preserve line structure by emitting newlines, even in inactive regions
- Complete skipping of inactive regions (no output, no expansion)
- Macro definitions inside active conditionals are processed normally
- Simple linear scan through conditional stack for state management
