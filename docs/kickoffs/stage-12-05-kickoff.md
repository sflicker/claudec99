# Stage-12-05 Kickoff: Pointer Return Types

## Spec Summary
Stage 12-05 extends the function-call boundary to support pointer return
types. A function may declare a pointer return type such as `int *f(...)`
or `int **f(...)`. The grammar widens the function rule so the leading
type may be `<integer_type> { "*" }`. Returned pointer values flow
through `rax` like long values. Type matching is strict: the return
expression's type chain must equal the function's declared return type
chain (no implicit pointer conversions, integer → pointer is rejected,
pointer literal `0` is rejected). Symmetrically, the call expression
carries the callee's return type so assignments / further returns at
the call site type-check the same way.

## What changes from Stage 12-04
- Grammar: `<function>` becomes
  `<type> <identifier> "(" [<parameter_list>] ")" ( <block_statement> | ";" )`.
- Parser: `parse_function_decl` consumes `<integer_type> { "*" }` for
  the return type, producing a `Type` chain. `AST_FUNCTION_DECL.full_type`
  is populated when the return is a pointer.
- Codegen: at `AST_RETURN_STATEMENT`, the integer-style `emit_convert`
  is skipped when either side is a pointer; instead strict type equality
  is enforced. At `AST_FUNCTION_CALL`, the call-result expression is
  annotated with the callee's pointer chain so downstream assignments /
  returns see the right type. Return-from-pointer-function path leaves
  the value in full `rax`.
- Pretty-printer: `AST_FUNCTION_DECL` renders pointer return types via
  `ast_print_type`.

## Spec issues / clarifications
1. **"Nested Pointer Return" test** — function is named `pd2` but its
   body returns `pp` (undefined; the parameter is `p`) and `main` calls
   `id2` (also undefined). Intent: a function returning `int **` from a
   pointer parameter. Test will use consistent names.
2. **"Invalid: return incorrect pointer type" test** — `main` does
   `return(&a);` where `a` is a `char` and `main` is `int`-returning,
   adding a second error unrelated to the spec's focus. Test will keep
   `main` minimal so the targeted compiler error is the pointer-return
   mismatch.
3. **"Invalid: returning non pointer" test** — same minor issue with
   `&a` in `main`. Test will keep `main` minimal.
4. **"Return Pointer Literal 0"** — explicitly out of scope; the
   integer literal `0` is rejected as a plain integer-vs-pointer type
   mismatch via the existing pointer/integer mismatch path.
5. The parser's `FuncSig` only stores `TypeKind` for the return type;
   the call site continues the stage 12-04 pattern of looking up the
   callee's `AST_FUNCTION_DECL` from `cg->tu_root` to recover
   `full_type` for strict chain matching.

## Planned Changes

1. **Tokenizer** — no changes.
2. **Parser** (`src/parser.c`)
   - In `parse_function_decl`, replace the call to
     `parser_expect_integer_type` with logic that consumes the integer
     base type, then zero or more `*` tokens, building a `Type` chain.
     When at least one `*` is consumed, set `func->decl_type =
     TYPE_POINTER` and `func->full_type` to the chain head; otherwise
     keep `decl_type` as the integer kind.
   - `FuncSig.return_type` continues to be a `TypeKind` (sufficient for
     pointer-vs-integer call-site checks).
3. **AST** — no new node types. `full_type` on `AST_FUNCTION_DECL` is
   populated for pointer returns.
4. **AST pretty-printer** (`src/ast_pretty_printer.c`)
   - In the `AST_FUNCTION_DECL` branch, when `decl_type == TYPE_POINTER`,
     render the return type via `ast_print_type`, mirroring
     `AST_PARAM` / `AST_DECLARATION`.
5. **Code generator** (`src/codegen.c`)
   - Add `Type *current_return_full_type` to `CodeGen` (parallel to
     `current_return_type`) so the return statement can do strict chain
     matching for pointer returns.
   - `codegen_function`: set `current_return_full_type = node->full_type`
     when the function is pointer-returning, otherwise `NULL`.
   - `AST_FUNCTION_CALL`: after the call, set `node->full_type` from the
     callee's `AST_FUNCTION_DECL.full_type` so pointer-typed call
     results carry their chain. `node->result_type` becomes
     `TYPE_POINTER` for pointer returns.
   - `AST_RETURN_STATEMENT`: when either the return-expression result
     type or `cg->current_return_type` is `TYPE_POINTER`, replace the
     `emit_convert` call with strict type checks: integer → pointer,
     pointer → integer, and mismatched pointer chains all error. On a
     matching pointer return, no conversion is emitted (value already
     in full `rax`).
6. **Tests**
   - Valid:
     - `test_pointer_return__7.c` — `int *identity(int *p) { return p; }`.
     - `test_pointer_return_nested__5.c` — `int **id2(int **pp) { return pp; }`.
   - Invalid:
     - `test_invalid_23__incompatible_pointer_return.c` — `int *`
       declared, `char *` returned.
     - `test_invalid_24__expected_pointer_return.c` — `int *` declared,
       integer (variable) returned.
     - `test_invalid_25__incompatible_pointer.c` — `int *` returned,
       assigned to `char *` (existing-style fragment).
     - `test_invalid_26__expected_integer.c` — assigning `int *` return
       to `int` local (treated as a declaration init type mismatch via
       the integer / pointer rule already in place for assignments).
     - `test_invalid_27__expected_pointer_return.c` — returning `0`
       from `int *` function.
     - `test_invalid_28__incompatible_pointer_return.c` — declared
       `int **`, returning `int *` (level mismatch).
   - Print-AST: add `test_stage_12_05_pointer_return.c` showing a
     pointer-returning function in the AST.
7. **Documentation**
   - Update `docs/grammar.md`: change `<function>` to use `<type>` for
     the return.
8. **Commit** at the end with a concise message.
