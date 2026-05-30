# Stage 78 Kickoff: General Postfix Expression Chaining

## Spec Summary

Stage 78 enables postfix operators to chain naturally, matching C's postfix-expression grammar model. The grammar already specifies a loop over postfix suffixes (subscript `[]`, member access `.`, arrow access `->`, function call `()`, and increment/decrement `++`/`--`), but the implementation has gaps:

1. The parser has an overly-restrictive type check on the subscript base that only allows `AST_VAR_REF`, `AST_DEREF`, and `AST_ARRAY_INDEX`
2. Code generation does not handle `AST_MEMBER_ACCESS` and `AST_ARROW_ACCESS` nodes as subscript bases

This stage enables expressions like:
- `p->values[0]` (subscript on arrow member)
- `n.values[0]` (subscript on dot member)
- `tokens[0].kind` (member access on subscript)
- `p->tokens[p->pos].kind` (complex chains)

## Spec Issues Identified

1. **Three test cases missing closing `}`**:
   - "Subscript of dot member array" (line 86)
   - "Pointer member subscript" (line 147)
   - "string/member/subscript chain" (line 166)

2. **"Nested member subscript" test has off-by-one error** (line 178):
   - Sets `p.pos = 4` but `int values[4]` only has valid indices 0–3
   - Should be `p.pos = 3` instead

3. **"Arrow member access after non-pointer subscript" invalid test has syntax error** (line 207–208):
   - Shows `struct S [` but should be `struct S {`

## Required Changes

### Tokenizer (`src/lexer.c`)
No changes needed.

### Parser (`src/parser.c`)
**Location**: `parse_postfix()` function, subscript (`TOKEN_LBRACKET`) case

**Current restriction**: Only allows `AST_VAR_REF`, `AST_DEREF`, and `AST_ARRAY_INDEX` as subscript base

**Change**: Expand type check to also permit:
- `AST_MEMBER_ACCESS`
- `AST_ARROW_ACCESS`

### AST (`include/ast.h`)
No new node types needed. `AST_MEMBER_ACCESS` and `AST_ARROW_ACCESS` already exist.

### Code Generation (`src/codegen.c`)
**Location**: `emit_array_index_addr()` function

**Add two new cases** before the final `AST_VAR_REF` branch:

1. **`AST_MEMBER_ACCESS` base**:
   - Call `emit_member_addr()` to put field address in `rax`
   - If field type is `TYPE_ARRAY`, rax is already the base address
   - If field type is `TYPE_POINTER`, emit `mov rax, [rax]` to load the pointer value
   - Then execute standard push/eval-index/scale/pop/add sequence

2. **`AST_ARROW_ACCESS` base**:
   - Same logic using `emit_arrow_addr()`

### Version (`src/version.c`)
Update `VERSION_STAGE` from `"00770000"` to `"00780000"`

### Grammar
No changes needed — `docs/grammar.md` already correctly specifies postfix expression as a loop over suffixes.

## Test Plan

### Valid Tests (8 planned)
1. Subscript of arrow member array
2. Subscript of dot member array
3. Member access after subscript
4. Arrow/member/subscript chain (complex)
5. Pointer member subscript
6. String/member/subscript chain
7. Nested member subscript expression as index
8. (one additional to be determined during implementation)

### Invalid Tests (4 planned)
1. Subscript non-array-pointer member
2. Dot member access after non-struct subscript
3. Arrow member access after non-pointer subscript
4. Missing member in chained expression

### Pretty Printer Test (1 planned)
Update pretty printer to display nested postfix expressions clearly, with test example using the complex arrow/member/subscript chain

## Implementation Order

1. Update `VERSION_STAGE` in `src/version.c`
2. Modify parser type check in `src/parser.c` to allow `AST_MEMBER_ACCESS` and `AST_ARROW_ACCESS`
3. Add two new cases in `src/codegen.c` `emit_array_index_addr()`
4. Implement/verify pretty printer support
5. Create test suite with valid and invalid cases
6. Run full test suite to verify no regressions

## Decisions

- No AST changes needed; reuse existing node types
- Focus on enabling the already-correct grammar model through parser and codegen fixes
- Test invalid cases to ensure proper error reporting
