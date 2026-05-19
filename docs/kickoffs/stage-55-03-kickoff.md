# Stage 55-03 Kickoff: Undefined identifiers in #if/#elif evaluate to 0

## Summary of the Spec

The preprocessor currently errors when encountering bare undefined identifiers in `#if` and `#elif` expressions. This stage changes the behavior so that:

- **Bare undefined identifiers** in `#if NAME` and `#elif NAME` evaluate to 0 (false)
- **Defined object-like macros** expanding to integer literals use their numeric value
- **Defined macros with other content** remain unsupported (error)

Examples:
- `#if UNDEFINED_NAME` → evaluates to false (0)
- `#if ENABLED` where `#define ENABLED 1` → evaluates to true (1)
- `#if DISABLED` where `#define DISABLED 0` → evaluates to false (0)
- `#define COMPLEX abc; #if COMPLEX` → error (unsupported)

## Required Tokenizer Changes

None. Tokenizer requires no changes.

## Required Parser Changes

None. Parser requires no changes.

## Required AST Changes

None. AST requires no changes.

## Required Code Generation Changes

None. Codegen requires no changes.

## Test Plan

### Modified behavior
1. Delete `test/invalid/test_pp_if_identifier__identifier_is_not_a_defined.c` — this test assumes undefined identifiers are an error, but they now evaluate to 0
2. Add `test/valid/test_pp_if_undefined_identifier__42.c` — undefined identifier MISSING in `#if` evaluates to false, else branch returns 42
3. Add `test/valid/test_pp_if_defined_zero_false__42.c` — macro FLAG defined as 0, `#if FLAG` evaluates to false, else branch returns 42
4. Add `test/valid/test_pp_if_undef_then_if__42.c` — macro FLAG defined as 1 then undefined, `#if FLAG` evaluates to false, else branch returns 42

### Implementation focus
The changes are localized to `src/preprocessor.c`:

- **Case: bare NAME in #if expression** (lines ~641-645)
  - When macro not found (!m), set `cond_val = 0` instead of exiting with error

- **Case: bare NAME in #elif expression** (lines ~730-733)
  - When macro not found (!m), set `cond_val = 0` instead of exiting with error

No changes to tokenizer, parser, AST, or code generation. No grammar changes.

## Spec Notes

The first In Scope example uses `#end` instead of `#endif` (line 13) — this is clearly a typo in the spec and should be `#endif`.
