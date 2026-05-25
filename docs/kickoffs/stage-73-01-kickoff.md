# Stage 73-01 Kickoff: Anonymous Struct and Union Type Declarations

## Spec Summary

Stage 73-01 adds support for anonymous struct and union type declarations. These allow structs and unions to be declared without a tag name, provided a body `{ ... }` is present. Both direct variable declarations and typedef forms are supported.

**Key examples:**
```c
struct { int x; int y; } p;
union { int i; char c; } v;
typedef struct { int x; int y; } Point;
```

**Type identity rule:** Each anonymous struct/union definition creates a unique type. Two anonymous types with identical layouts are not assignment-compatible. However, anonymous types can be shared through typedef.

```c
struct { int x; } a;
struct { int x; } b;
a = b;  // INVALID: different anonymous struct types
```

```c
typedef struct { int x; } S;
S a, b;
a = b;  // VALID: same typedef-defined type
```

## Grammar Updates

**struct_specifier** and **union_specifier** rules updated to make tag optional when body is present:

```ebnf
<struct_specifier> ::= "struct" [ <identifier> ] "{" <struct_declaration_list> "}"
                     | "struct" <identifier>

<union_specifier> ::= "union" [ <identifier> ] "{" <struct_declaration_list> "}"
                    | "union" <identifier>
```

**Constraint:** Tag is required when no body is present (struct/union references must have a tag).

## Required Changes

### Parser (`src/parser.c`)
- Modify `parse_struct_specifier()` to make tag optional when `{` follows
- Modify `parse_union_specifier()` to make tag optional when `{` follows
- Add error when neither tag nor body is present (e.g., `struct *p;` or `union *p;`)

### No New Tokens or AST Types
- Anonymous structs/unions use existing `TYPE_STRUCT` and `TYPE_UNION`
- Type uniqueness is enforced through pointer identity comparison (each anonymous type is a distinct object in memory)

### No Codegen Changes Needed
- Existing codegen handles anonymous types correctly via the type system

### Documentation (`docs/grammar.md`)
- Update struct_specifier and union_specifier rules

### Version (`src/version.c`)
- Update `VERSION_STAGE` to `"00730100"`

### Tests
- **Valid tests (7 new):**
  - Anonymous struct with direct declaration
  - Anonymous struct with multiple variables
  - Anonymous struct with typedef
  - Anonymous union with direct declaration
  - Anonymous union with multiple variables
  - Anonymous union with typedef
  - Anonymous union with multiple typedefs

- **Invalid tests (3 new):**
  - `struct *p;` (no tag, no body)
  - `union *p;` (no tag, no body)
  - Assignment between different anonymous types with identical layouts

## Ambiguities and Decisions

1. **Spec syntax issues:** Several test examples in the spec are missing closing `}` for `main()` function body (lines 129, 139) and use typo "expstec" instead of "expect" (line 139). Kickoff tests will use correct C syntax.

2. **Type identity mechanism:** Relies on existing pointer-identity comparison in the type system. Each anonymous type is a new `struct type_s` object created during parsing, so they are automatically distinct.

3. **Out of scope:** C11 anonymous struct/union members (designated initializers with unnamed members). This stage only supports simple anonymous types.

## Implementation Order

1. Implement parser changes (`parse_struct_specifier` and `parse_union_specifier`)
2. Create valid and invalid tests
3. Update grammar documentation
4. Update version.c
5. Test execution and validation
