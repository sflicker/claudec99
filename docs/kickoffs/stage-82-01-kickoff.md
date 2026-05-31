# Stage 82-01 Kickoff: `const` Qualifiers in Struct Members and Type Names

## Summary of the Spec

Stage 82-01 extends `const` qualifier support into struct and union member declarations. The project already supports `const` in ordinary declarations and function parameters with const-assignment checks and pointer const-discard diagnostics. This stage closes a self-hosting gap by allowing `const` in two forms:

1. **Struct/union members with const-qualified base types**:
   ```C
   struct Field {
       const char *name;      // pointer to const char
       const int kind;        // const int member
   };
   ```

2. **Type-name contexts** (addressed in later substages):
   ```C
   sizeof(const char *);      // type-name with const qualifier
   ```

This substage focuses on struct and union member declarations only.

### Key Distinction
- `const char *name` = pointer to const char (qualifier on base type)
- `char * const name` = const pointer to mutable char (qualifier on pointer)

Both forms should preserve the qualifier in the member type.

## Required Tokenizer Changes

**None.** TOKEN_CONST already exists and is already recognized during tokenization.

## Required Parser Changes

**src/parser.c** — Two specifier parsing functions require updates:

1. **`parse_struct_specifier`**:
   - Before calling `parse_type_specifier`, consume an optional TOKEN_CONST and set `field_is_const = 1`
   - After parsing type specifier and declarator, apply const qualifier:
     - If `field_is_const && pointer_count > 0`: use `type_const_copy(field_base)` to make the base type const
     - If `field_is_const && pointer_count == 0`: set `tmp_fields[n_fields].is_const = 1` on the struct field
     - Also propagate `d.pointer_is_const` to `tmp_fields[n_fields].is_const` for pointer-to-const cases

2. **`parse_union_specifier`**:
   - Apply the same const-consuming and qualifier-propagation logic as in struct

## Required AST Changes

**None.** The AST structures are sufficient; they already support const qualifiers on types.

## Required Type System Changes

**include/type.h**:
- Add `int is_const;` field to the `StructField` struct to track whether a member is const-qualified

## Required Code Generation Changes

**src/codegen.c** — Two member assignment paths require const-assignment checks:

1. **Struct member assignment** (`AST_MEMBER_ACCESS`, around line 1626):
   - After calling `emit_member_addr` to get the member address
   - Check `if (f->is_const)` and emit a compile error message for const member assignment

2. **Arrow/union member assignment** (`AST_ARROW_ACCESS`, around line 1658):
   - Apply the same const-assignment check to union and struct pointer member assignments

## Test Plan

### Valid Tests

1. **Const pointer member — assignment and dereference**:
   ```C
   struct Field {
       const char *name;
   };
   
   int main(void) {
       struct Field f;
       f.name = "hello";
       return f.name[0] == 'h';     // expect 1
   }
   ```

2. **Const scalar member — initialization and read**:
   ```C
   struct Field {
       const int kind;
   };
   
   int main(void) {
       struct Field f = {42};
       return f.kind;               // expect 42
   }
   ```

### Invalid Tests

1. **Const scalar member — assignment rejection**:
   ```C
   struct Field {
       const int kind;
   };
   
   int main(void) {
       struct Field f = {1};
       f.kind = 2;                  // INVALID: assignment to const member
       return f.kind;
   }
   ```

## Implementation Order

1. **include/type.h** — Add `is_const` field to `StructField`
2. **src/parser.c** — Implement const parsing in `parse_struct_specifier` and `parse_union_specifier`
3. **src/codegen.c** — Implement const-assignment checks in member assignment paths
4. **Test suite** — Add valid and invalid const-member tests
5. **src/version.c** — Update version identifier

## Ambiguities and Decisions

- **Member-level const vs. type-level const**: The implementation treats `const int kind;` as a const member (set via `is_const`), while `const char *name;` sets const on the pointed-to type. Both approaches are handled via the existing type system's const machinery.

- **Error messages**: Const member assignments should emit a clear error message similar to existing const-discard diagnostics.

- **Union members**: Union members follow the same const rules as struct members; const assignments to union members are rejected with the same error message.
