# Stage 32 Kickoff: Aggregate Initializer Lists

## Summary

Stage 32 adds aggregate initializer lists for local array and struct variables. This stage introduces brace-enclosed initializers like `{ expr, ... }` that populate arrays and structs element-by-element, with partial initialization automatically zero-filling remaining elements and members.

## Tokenizer Changes

**No new tokens required.**

The tokenizer already supports `TOKEN_LBRACE` (`{`) and `TOKEN_RBRACE` (`}`), which are sufficient for aggregate initializers.

## Grammar Changes

**Update `init_declarator` and related rules:**

Replace:
```ebnf
<init_declarator> ::= <declarator> [ "=" <initializer_expression> ]

<initializer_expression> ::= <assignment_expression>
```

With:
```ebnf
<init_declarator> ::= <declarator> [ "=" <initializer> ]

<initializer> ::= <assignment_expression>
                | "{" <initializer_list> [ "," ] "}"

<initializer_list> ::= <initializer> { "," <initializer> }
```

This allows initializers to be either a simple assignment expression or a brace-enclosed list of initializers.

## AST Changes

**New node type:**
- Add `AST_INITIALIZER_LIST` to `include/ast.h`

**Node structure:**
- `child_count` = number of initializers in the list
- `children[0..child_count-1]` = AST nodes for each initializer expression

## Parser Changes

**Helper function:**
- Add `parse_initializer()` to handle both assignment expressions and brace-enclosed initializer lists
  - If next token is `TOKEN_LBRACE`, parse a list (creating `AST_INITIALIZER_LIST`)
  - Otherwise, parse and return an assignment expression

**Init declarator parsing:**
- Update `parse_init_declarator()` to call `parse_initializer()` instead of directly parsing `initializer_expression`

**Semantic checks:**
- Initializer lists may only be used with array or struct declarators (not scalars)
- This check happens in the parser or codegen

## Code Generation Changes

**Array declaration with initializer list:**
- If initializer is `AST_INITIALIZER_LIST`, iterate through child expressions
- Allocate space for the array (size × element_size)
- Store initializer values at appropriate offsets
- Zero-fill remaining elements (from initializer count to array size)

**Struct declaration with initializer list:**
- If initializer is `AST_INITIALIZER_LIST`, iterate through child expressions
- Store each initializer value at the corresponding struct field's offset
- Zero-fill remaining fields (from initializer count to field count)

**Scalar declaration with initializer list:**
- Error: initializer lists are not supported for scalar types (or silently unsupported; check spec intent)

**Helper considerations:**
- Track current element/field index during initialization
- Use field layout information from struct type to place values correctly

## Test Plan

**Valid Tests:**

1. `test_array_initializer_list__6.c` — Array initialization with all elements
   ```c
   int main() {
       int a[3] = { 1, 2, 3 };
       return a[0] + a[1] + a[2];   // expect 6
   }
   ```

2. `test_struct_initializer_list__7.c` — Struct initialization with all fields
   ```c
   struct Point {
       int x;
       int y;
   };
   
   int main() {
       struct Point p = { 3, 4 };
       return p.x + p.y;   // expect 7
   }
   ```

3. `test_array_partial_initializer__5.c` — Array initialization with zero-fill
   ```c
   int main() {
       int a[3] = { 5 };
       return a[0] + a[1] + a[2];   // expect 5
   }
   ```

## Known Spec Issues

1. **Spec typo in partial initializer test**: The closing brace uses `]` instead of `}`. Correct this to use `}` in the implementation.

## Implementation Order

1. **AST**: Add `AST_INITIALIZER_LIST` node type
2. **Parser**: Implement `parse_initializer()` helper and integrate with `parse_init_declarator()`
3. **Code generation**: Handle `AST_INITIALIZER_LIST` in array and struct declaration codegen
4. **Tests**: Create and verify the three test cases
5. **Documentation**: Update `docs/grammar.md` with the new grammar rules
