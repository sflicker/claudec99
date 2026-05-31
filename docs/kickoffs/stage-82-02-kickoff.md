# Stage 82-02 Kickoff: Const-Qualified Member Lvalue Rules

## Summary of the Spec

Stage 82-02 enforces const-qualification rules for pointer-to-const struct members accessed via subscript. The project already handles direct const member assignment rejection (stage 82-01) and deref-assignment through const pointers (stage 66). This stage closes a gap by rejecting array subscript writes through const-qualified pointer members.

**Scenario**: When a struct member is declared `const char *p` (pointer to const char), the member itself (`p`) is assignable if the struct is mutable:
```C
s.p = "hello";   // OK
```

However, writing through that pointer must be rejected:
```C
*s.p = 'H';      // Already caught (stage 66)
s.p[0] = 'H';    // NOT YET CAUGHT — this stage
```

## Required Tokenizer Changes

**None.** All necessary tokens already exist.

## Required Parser Changes

**None.** The parser already correctly identifies array subscript expressions on member access expressions.

## Required AST Changes

**None.** The AST already represents `s.p[0]` as `AST_ASSIGNMENT` with `AST_ARRAY_INDEX` operand, with full type information available.

## Required Code Generation Changes

**src/codegen.c** — Member array assignment path requires one new check:

1. **Array subscript assignment through member pointer** (around line 1588, `AST_ASSIGNMENT + AST_ARRAY_INDEX` path):
   - After `emit_array_index_addr` returns the element type, check `if (element->is_const)`
   - If true, emit compile error: "assignment through pointer to const-qualified type"

## Test Plan

### Valid Tests

1. **Pointer-to-const member assignment and read**:
   ```C
   struct S {
       const char *p;
   };
   
   int main() {
       struct S s;
       s.p = "hello";
       return s.p[1] == 'e';     // expect 1
   }
   ```

### Invalid Tests

1. **Array subscript write through const-qualified pointer member**:
   ```C
   struct S {
       const char *p;
   };
   
   int main(void) {
       struct S s;
       s.p = "hello";
       s.p[0] = 'H';             // INVALID
       return 0;
   }
   ```

## Implementation Order

1. **src/codegen.c** — Add const-ptr-member subscript write check
2. **Test suite** — Add valid and invalid const-member-subscript tests
3. **src/version.c** — Update version identifier

## Ambiguities and Decisions

- **Error consistency**: The error message "assignment through pointer to const-qualified type" matches the existing stage 66 deref-assignment check to maintain consistency across const-qualification diagnostics.

- **Scope**: This stage focuses only on the subscript write case. The dereference write case is already handled by stage 66's existing const-pointer checks.
