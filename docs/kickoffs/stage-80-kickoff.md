# stage-80 Kickoff

## Summary of the spec

Extend prefix/postfix `++` and `--` operators to work on any modifiable lvalue, not just bare identifiers. Currently the compiler accepts `i++` and `--i`, but rejects `p->len++`, `arr[i]++`, `--x->n`, `(*p)++`, and `tokens[i].kind++`. The two critical self-compile blockers are classic C idioms: `parent->children[parent->child_count++] = child` and `g->data[g->len++] = c`.

Semantics:
- Operand must be a modifiable lvalue of arithmetic or pointer type
- Postfix returns the old value; prefix returns the new value
- Pointer increment/decrement scales by pointee size
- Invalid operands (non-lvalues like `42++` or `(a+b)++`, and const lvalues) must be rejected with a clear error

## Required tokenizer changes

None. `++` and `--` are already tokenized.

## Required parser changes

Currently `parse_postfix` and `parse_unary` (in `src/parser.c`) enforce that the operand of `++`/`--` must be `AST_VAR_REF` only. This check must be removed to allow any postfix/unary expression as the operand. Lvalue validation is deferred to codegen (per spec: "valid lvalue after parsing/type checking").

Changes:
- In `parse_postfix`: remove the `ast_kind != AST_VAR_REF` rejection; allow `++`/`--` to attach to any already-parsed postfix expression
- In `parse_unary`: remove the `ast_kind != AST_VAR_REF` rejection for prefix `++`/`--`; allow attachment to any unary expression

## Required AST changes

None. Reuse existing `AST_PREFIX_INC_DEC` and `AST_POSTFIX_INC_DEC` node types.

## Required code generation changes

The current codegen in `src/codegen.c` handles `AST_VAR_REF` operands directly by reading `children[0]->value` (the identifier). For general lvalues (array subscript, struct member access, pointer member access, dereference), a new helper `codegen_inc_dec_general()` is needed.

Strategy:
- Add `codegen_inc_dec_general()` to handle `AST_ARRAY_INDEX`, `AST_MEMBER_ACCESS`, `AST_ARROW_ACCESS`, and `AST_DEREF` operands
- This helper reuses stage-79 address machinery: `emit_array_index_addr`, `emit_member_addr`, `emit_arrow_addr`
- The helper computes the lvalue address once, loads the element value (with appropriate sign/zero extension), adjusts by the step size (1 for integers, pointee size for pointers), stores back, and yields either the old value (postfix) or new value (prefix)
- For non-lvalue operands, raise "operand of ++/-- must be a modifiable lvalue"
- Dispatch to the general helper from prefix/postfix branches when operand is not `AST_VAR_REF`; retain the fast path for identifiers
- Add const rejection to the identifier path

Update `version.c`: bump `VERSION_STAGE` from `00790000` to `00800000`.

## Test plan

Valid tests:
- Arrow member postfix increment: `p->cap++`
- Prefix decrement on arrow member: `--p->n`
- Array element increment: `xs[1]++`
- Nested self-compile blocker (array element within member): `parent->children[parent->child_count++] = child`
- Nested self-compile blocker (array element with length field): `g->data[g->len++] = c`
- Postfix returns old value: `y = x++` should assign 41 while `x` becomes 42
- Prefix returns new value: `y = ++x` should assign 42

Invalid tests:
- Literal increment: `42++` (non-lvalue)
- Expression increment: `(a + b)++` (non-lvalue)
- Const lvalue increment: `const int x = 1; x++;` (non-modifiable lvalue)

All existing tests must continue to pass.

## Implementation order

1. Parser: Remove AST_VAR_REF-only checks in `parse_postfix` and `parse_unary`
2. Codegen: Add `codegen_inc_dec_general()` helper and dispatch logic; add const rejection to identifier path
3. Version: Bump VERSION_STAGE
4. Tests: Add valid and invalid test cases
5. Docs: Update docs/grammar.md and README.md

## Known ambiguities and decisions

**Spec test issue**: The "Member postfix increment" test in the spec uses `p->cat++` (typo) instead of `p->cap++` (the struct field is `cap`), and is missing a closing brace. This will be corrected in the actual test.

**Const test**: The spec's invalid test uses `const x = 1;` (implicit int, unsupported by this compiler). The actual invalid test will use `const int x = 1; x++;` to properly test const rejection.

**Design decision**: Keep the existing identifier codegen path unchanged (well-tested, handles static vars) and add a separate general-lvalue path to minimize regression risk.

**Design decision**: Reuse stage-79 address helpers (`emit_array_index_addr`, `emit_member_addr`, `emit_arrow_addr`) rather than introducing a new unified lvalue-address function, to leverage proven infrastructure.
