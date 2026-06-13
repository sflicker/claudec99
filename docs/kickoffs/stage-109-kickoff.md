# Stage 109 Kickoff: Float and Double Types, Literals, and Stack Variables

## Spec Summary

Stage 109 introduces `float` and `double` as first-class types to the ClaudeC99 compiler. Variables of these types can be declared, initialized, copied, stored as struct/union members, and held as global variables. Floating-point literals (both `float` with `f`/`F` suffix and `double`) are lexed and stored as read-only data in `.rodata`. Implicit float-to-double widening is supported. Arithmetic, comparisons, and function parameters/return values are deferred to Stages 110–112.

**Scope**:
- Lexer: `float`/`double` keywords, FP literal scanning (decimal with optional `.` and/or exponent; optional `f`/`F` suffix; also leading-dot forms like `.5`)
- Type system: `TYPE_FLOAT` and `TYPE_DOUBLE` singletons (4/8 bytes, 4/8 byte alignment)
- Parser: type-specifier recognition, literal parsing, implicit widening in assignment/declaration paths
- Codegen: stack allocation, `movss`/`movsd` load/store, `.rodata` FP literal pool with deduplication, `cvtss2sd` widening
- Tests: 6 new tests covering declare, copy, widening, struct members, and globals

---

## Files to Change

| File | Changes |
|------|---------|
| `include/token.h` | Add `TOKEN_FLOAT`, `TOKEN_DOUBLE`, `TOKEN_FLOAT_LITERAL`, `TOKEN_DOUBLE_LITERAL` |
| `include/type.h` | Add `TYPE_FLOAT`, `TYPE_DOUBLE` to `TypeKind`; declare `type_float()`, `type_double()` |
| `include/ast.h` | Add `AST_FLOAT_LITERAL` node type |
| `include/codegen.h` | Add `Vec fp_literals` field to `CodeGen` struct (for `.rodata` literal pool) |
| `src/lexer.c` | Keyword recognition and FP literal lexing |
| `src/type.c` | Type singletons, size/align, `type_is_fp()` predicate, `type_kind_name()` update |
| `src/parser.c` | `parse_type_specifier()` for `TOKEN_FLOAT`/`TOKEN_DOUBLE`; `parse_primary()` for literals; lookahead set updates; implicit widening validation |
| `src/codegen.c` | Stack/global allocation, load/store emitters, `.rodata` literal emission, implicit widening codegen |
| `src/ast_pretty_printer.c` | `AST_FLOAT_LITERAL` case |
| `src/version.c` | Bump stage to `01090000` |

---

## Key Design Decisions

### 1. Single AST Node Type for Both Float and Double
- Use `AST_FLOAT_LITERAL` for both types; distinguish via `node->decl_type` (`TYPE_FLOAT` or `TYPE_DOUBLE`)
- Parser sets `result_type` to the appropriate singleton (`type_float()` or `type_double()`)

### 2. Floating-Point Literal Pool
- Maintain a `Vec` of `{const char *raw_text, int label_index, int is_double}` in `CodeGen`
- Search before emitting; reuse label if identical text is found (conservative deduplication)
- Emit to `.rodata` with `Lfc_<N>: DD <decimal>` (float) or `DQ <decimal>` (double)
- NASM handles decimal-to-IEEE 754 conversion; strip `f`/`F` suffix before emitting

### 3. Result Register Convention
- FP values land in `xmm0` (not `rax`)
- Integer/pointer values continue to use `rax`
- All FP load/store use SSE instructions (`movss`/`movsd`)

### 4. Implicit Float-to-Double Widening
- `float` rvalue → `double` lvalue: emit `cvtss2sd xmm0, xmm0` before store
- Reverse (double → float) requires explicit cast, rejected silently for now
- Only widening conversion allowed; narrowing not implicit

### 5. Stack and Global Allocation
- Use existing `type_size()` and `type_align()` infrastructure
- FP locals reserve 4/8 bytes with 4/8 byte alignment (naturally aligned)
- FP globals emitted to `.data` (initialized) or `.bss` (uninitialized)

---

## Implementation Order

1. **Token layer**: Add four new token types; update lexer for keywords and FP literals
2. **Type system**: Add `TYPE_FLOAT`/`TYPE_DOUBLE`; define singletons and accessors
3. **AST**: Define `AST_FLOAT_LITERAL` node
4. **Parser**: Update `parse_type_specifier()`, `parse_primary()`, lookahead sets, implicit widening
5. **Codegen**: Allocate registers, emit load/store, build FP literal pool, emit `.rodata` section
6. **Tests**: Create 6 new test cases
7. **Self-host**: Full C0→C1→C2 cycle

---

## Lookahead Set Updates Required

All locations that test "is this a type specifier?" must include `TOKEN_FLOAT` and `TOKEN_DOUBLE`:
- `parse_type_specifier()` (primary site)
- `parse_cast()` — postfix compound-literal form
- `parse_postfix()` — compound-literal form
- `parse_statement()` — declaration dispatcher
- Block-scope dispatch
- File-scope dispatch
- `eval_const_primary()` in `parse_statement()` (sizeof argument)

---

## Spec Ambiguities and Clarifications

1. **Leading-dot floats**: `.5` is lexed as `TOKEN_DOUBLE_LITERAL` (equivalent to `0.5`)
2. **Literal deduplication**: Two textually identical literals share one label; textually distinct literals with the same IEEE 754 value get separate labels (conservative but correct)
3. **Global FP initialization**: File-scope FP globals accept only FP literal initializers (verified at parse time via `AST_FLOAT_LITERAL` check)
4. **No implicit int→float**: Integer-to-float conversions are not implicit (deferred to Stage 110)
5. **No FP in function calls**: Float/double parameters and return values deferred to Stage 112

---

## Summary of Deliverables

- Kickoff (this document)
- Milestone summary (after testing passes)
- Transcript (after all implementation and testing)
- Grammar update (add float/double to type-specifiers and floating-constant production)
- Checklist entry (Stage 109 section)
- README update (through stage line, type support, test totals)
