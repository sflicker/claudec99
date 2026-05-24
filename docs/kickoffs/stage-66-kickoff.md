# Stage 66 Kickoff: Const Pointer Compatibility Hardening

## Summary of the Spec

Stage 66 adds const-qualifier enforcement at the pointer level. The compiler must enforce four distinct behaviors:

1. **Write through pointer-to-const**: `*p = 2` where `const int *p` → always an error
2. **Assignment to const pointer**: `p = &x` where `char * const p` → always an error
3. **Const discard in pointer assignment**: `char *q = const_char_p` → warning; error with `-Werror` flag
4. **Adding const**: `const char *p = char_q` → allowed, no diagnostic

## Required Tokenizer Changes

None. TOKEN_CONST already exists and is sufficient.

## Required Parser Changes

- Add `pointer_is_const` boolean to ParsedDeclarator struct
- Consume `const` token after `*` in parse_declarator()
- When building full_type for `const T *p`, use type_const_copy(base) for the base type
- Update decl->is_const for the pointer-is-const case (`T * const p`)
- Update both local-scope and file-scope declaration parsing functions to handle these cases

## Required AST Changes

None. The is_const field on ASTNode is sufficient for tracking const semantics.

## Required Code Generation Changes

- Add `warnings_are_errors` flag to CodeGen struct
- Add `codegen_warn()` helper function
- Update local_var_type() and global_var_type() to return const copy for is_const scalar variables
- In dereference-LHS assignment: check base->is_const → error ("assignment through pointer to const")
- In local/global variable pointer assignment: check for const-discard → warn/error
- In declaration initialization: check for const-discard
- In address-of (&var): propagate const from variable to result type

## Test Plan

### Valid Tests
- **test_const_ptr_add_const__0.c** — adding const to pointer assignment, no warning
- **test_const_ptr_discard_warning__0.c** — warning emitted but exit code 0

### Invalid Tests
- **test_const_ptr_write_through__assignment_through_pointer_to_const.c** — write through `const int *p`
- **test_const_ptr_const_pointer_assign__assignment_to_const_variable.c** — assignment to `T * const p`

### Integration Tests
- **const_ptr_discard_werror/** — const-discard with `-Werror` flag must compile error
- **const_addr_discard_werror/** — `int *p = &const_int_x` with `-Werror` must compile error

## Ambiguities and Decisions

- **Flag name**: Using `-Werror` (GCC convention); spec does not mandate specific flag name
- **No-flag behavior**: Tests that emit warnings but should succeed use the valid suite (exit 0)
- **Grammar**: No changes to grammar.md required
- **Function parameters**: Const propagation for function parameters is out of scope and not tested
- **Implementation order**: Parser changes first (pointer_is_const tracking), then code generation (diagnostics and type propagation)
