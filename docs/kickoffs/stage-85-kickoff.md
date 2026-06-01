# stage-85 Kickoff

## Summary of the spec

A struct/union member whose type is an array must decay to a pointer to its first element when used in a value context—specifically when passed to a function that takes a pointer parameter.

Covers both `.` (dot) access (`f(s.name)`) and `->` (arrow) access (`f(p->name)`), where `name` is a `char[32]` member.

Additionally, both spec tests begin with `struct S s = {"Hello"};`, which initializes a `char[32]` member from a string literal. This capability is currently rejected by the compiler. Treating the spec tests as authoritative, this stage also implements char-array struct-member string-literal initialization.

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation changes

1. **Array-member-to-pointer decay in value contexts** (src/codegen.c)
   - In `codegen_expression`, handle `AST_MEMBER_ACCESS` and `AST_ARROW_ACCESS` value paths
   - When the field kind is `TYPE_ARRAY`, the field address (already computed) becomes the decayed pointer value rather than loading from it
   - Set `result_type = TYPE_POINTER` and `full_type = type_pointer(element_type)`

2. **Update type inference** (src/codegen.c)
   - Matching cases in `expr_result_type` so call-site pointer compatibility checks see the pointer type

3. **String-literal initialization of char-array struct members** (src/codegen.c)
   - In `emit_local_struct_init`, copy string bytes into the field slot for `char[N]` members initialized from string literals

4. **Version bump** (src/version.c)
   - Update `VERSION_STAGE` to `"00850000"`

## Test plan

Two new tests from the spec:
- Test 1: Dot access to array member (`f(s.name)`)
  - Exit code: 42
  - Expected output: `Hello`
  
- Test 2: Arrow access to array member (`f(p->name)`)
  - Exit code: 42
  - Expected output: `Hello`

## Implementation order

1. Tokenizer (none)
2. Parser (none)
3. AST (none)
4. Codegen
5. Tests
6. Docs
7. Commit

## Key notes

- This is a codegen-only semantic stage with no grammar change
- Array decay at member access differs from function parameter array-to-pointer conversion because it happens at the point of member access, not at function call boundary
- Both string-literal initialization and array decay must work together for the spec tests to pass
