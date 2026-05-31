# stage-79 Kickoff: General Lvalue Compound Assignment

## Summary

Stage 79 generalizes compound-assignment operators (`+=`, `-=`, `*=`, `/=`, `%=`, `<<=`, `>>=`, `&=`, `^=`, `|=`) so they work on ANY lvalue, not just bare identifiers. Currently, expressions like `g->cap *= 2;`, `arr[i] += 2;`, `tokens[i].kind += 1;`, and `*p *= 2;` fail to parse with the error `expected ';', got '*='` because compound assignment is only handled in the bare-identifier fast-path of `parse_assignment_expression`. The fix desugars `lhs op= rhs` conceptually into `lhs = lhs op rhs` for all general lvalues (dereference, member access, arrow access, and array subscript).

## Derived Stage Values

| Key | Value |
|-----|-------|
| STAGE_LABEL | stage-79 |
| STAGE_DISPLAY | stage-79 |
| VERSION_STAGE | 00790000 |

## Required Changes

### Tokenizer

No changes required. All compound-assignment operator tokens already exist.

### Parser (src/parser.c)

The `parse_assignment_expression()` function currently has a bare-identifier fast-path that recognizes compound operators. Extend the general-lvalue branch (the path after `parse_conditional` that currently only recognizes plain `=`) to also recognize compound-assignment operator tokens.

Implementation approach:
- After parsing the conditional expression (lhs), check for either `=` or a compound-assignment operator.
- For compound operators, desugar `lhs op= rhs` into an `AST_ASSIGNMENT` with two children: the lhs and a binary operation `clone(lhs) op rhs`.
- The binary operation node contains the operator without the `=` (e.g., `*=` becomes `*`).

Keep the existing bare-identifier fast-path unchanged to preserve existing AST shapes and print tests.

### AST (include/ast.h + src/ast.c)

Add `ast_clone()` deep-copy helper function to safely duplicate an lvalue subtree, allowing the same lhs to appear both on the left side of assignment and as the left operand of the binary operation within the rvalue.

### Code Generation (include/codegen.h + src/codegen.c)

No changes required. The existing `gen_assignment()` already handles two-child general-lvalue assignments (Stage 33), and binary-operation codegen already exists.

### Tests

Add three valid integration tests:

1. **Struct-pointer compound assignment** (`p->cap *= 2` => 42)
   - Declare struct with cap field
   - Create pointer to struct, set cap to 21
   - Apply `*=` operator
   - Return the result

2. **Array-member compound assignment** (`tokens[i].kind += 2` => 42)
   - Declare struct Token with kind field
   - Create array of Token
   - Set `tokens[1].kind = 40`
   - Apply `+=` operator to `tokens[1].kind`
   - Return the result

3. **Pointer-dereference compound assignment** (`*p *= 2` => 42)
   - Declare integer and pointer
   - Set value to 21 via pointer
   - Apply `*=` operator via dereference
   - Return the result

### Documentation

- Update README compound-assignment bullet to clarify support for general lvalues (not just identifiers).
- Update `docs/grammar.md` if grammar wording changes as a result of this work.

## Known Ambiguities and Spec Issues

1. The array/member test in the spec references `tokens[i]` without declaring `i`, includes a missing closing brace, and has a mismatched reference to `tokens[1]`. This will be corrected to a syntactically valid test that properly initializes and uses the array index.

2. The README currently mentions compound assignment "on integer variables"; this will be refined to describe support for general lvalues.

## Implementation Order

1. Add `ast_clone()` helper in `include/ast.h` and `src/ast.c`.
2. Extend `parse_assignment_expression()` in `src/parser.c` to handle compound operators on general lvalues.
3. Add integration tests with corrected syntax.
4. Update README and `docs/grammar.md` as needed.
5. Run tests and validate.
6. Version bump and commit.

## Key Decisions

- **Preserve existing fast-path**: Keep the bare-identifier compound-assignment path unchanged to maintain existing AST shapes and test compatibility.
- **Desugar via binary operation**: Represent `lhs op= rhs` as `AST_ASSIGNMENT(lhs, AST_BINARY_OP(clone(lhs), op, rhs))` to allow code generation to reuse existing logic.
- **Dual evaluation**: The desugaring evaluates the lvalue twice, which matches the spec's stated conceptual model and is semantically correct for C (side effects in subscript/member access will occur twice, but this is consistent with the reference behavior).
- **No codegen changes**: Leverage existing assignment and binary-operation code generation pathways.
