# Stage-12-02 Kickoff: Address-of and Dereference

## Spec Summary
Stage 12-02 adds the unary address-of (`&expr`) and dereference (`*expr`)
operators, just enough to take a pointer to a variable, store it in a
pointer local, and read through it. Pointer assignment through
dereference (`*p = value`), `&*p` edge cases, pointer arithmetic, and
pointer comparisons remain out of scope.

## What changes from Stage 12-01
Stage 12-01 only introduced pointer **types**: pointer declarations
parse, pointer locals reserve 8 bytes, but no operator produces or
consumes a pointer **value**. Stage 12-02 adds those operators, which
also requires expression-level propagation of pointer Type information
(so `*p` knows the load width, and so a pointer-valued initializer
stores 8 bytes into a pointer local without being sign-extended).

## Spec issues / clarifications
1. **Typo**: the example shape lists `AST_ADDR_OR`; this is meant to be
   `AST_ADDR_OF`.
2. **Typos in the "Nested dereference" example**: missing `;` after
   `int **pp = &p`, missing `}` at end of `main`, and the return is
   `**p` where `p` is `int *` (so `**p` would be a type error). The
   test is implemented as `return **pp;`.
3. **Typo**: "dreference" → "dereference" in the invalid test heading.
4. **Missing `}`** in the "Basic address-of + dereference" snippet;
   implementation follows the corrected layout.
5. The spec says address-of requires an lvalue. The only lvalue
   available in this stage is a variable reference (`AST_VAR_REF`),
   so the parser rejects `&(x+1)`, `&5`, etc.

## Planned Changes

1. **Tokenizer** (`include/token.h`, `src/lexer.c`)
   - Add `TOKEN_AMPERSAND`. After the existing `&&` check, emit
     `TOKEN_AMPERSAND` for a bare `&`.
2. **AST** (`include/ast.h`)
   - Add `AST_ADDR_OF` and `AST_DEREF` to `ASTNodeType`.
   - Reuse the existing `Type *full_type` slot on `ASTNode` to carry
     the result Type for pointer-valued expressions in addition to its
     current declaration use.
3. **Parser** (`src/parser.c`)
   - Extend `parse_unary` to handle `TOKEN_AMPERSAND` (build
     `AST_ADDR_OF`; reject operand if not `AST_VAR_REF`) and
     `TOKEN_STAR` (build `AST_DEREF`).
   - Binary `*` continues to work because `parse_term` only invokes
     `parse_cast`/`parse_unary` after consuming a left operand — so
     unary `*` only fires when `*` starts an expression.
4. **Code generator** (`include/codegen.h`, `src/codegen.c`)
   - Extend `LocalVar` with `TypeKind kind` and `Type *full_type`.
   - `AST_VAR_REF` for a pointer local: load 8 bytes, set
     `result_type = TYPE_POINTER`, propagate `full_type`.
   - `AST_ADDR_OF`: look up the operand variable, emit
     `lea rax, [rbp - offset]`, set `result_type = TYPE_POINTER` and
     `full_type = type_pointer(<var type>)`.
   - `AST_DEREF`: codegen operand into `rax`; require operand
     `full_type->kind == TYPE_POINTER`; emit a size-appropriate load
     from `[rax]` based on `base->size`; set `result_type = base->kind`
     (and propagate `full_type` when the base is itself a pointer).
   - `AST_DECLARATION` initializer and `AST_ASSIGNMENT`: treat
     `result_type == TYPE_POINTER` as "value already in full rax"
     (`src_is_long = 1` on the size-8 store).
5. **Pretty printer** (`src/ast_pretty_printer.c`)
   - Add `AST_ADDR_OF` ("AddressOf") and `AST_DEREF` ("Dereference").
6. **Tests**
   - `test/valid/`: basic address-of + dereference, assignment-through-
     pointer-variable, nested dereference (`**pp`).
   - `test/invalid/`: dereferencing a non-pointer; address-of a
     non-lvalue.
   - `test/print_ast/`: a printer test exercising `&` and `*`.
7. **Documentation**
   - Update `docs/grammar.md`: extend `<unary_expression>` so the
     unary operator set includes `*` and `&`.
