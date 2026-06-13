# Stage 109 — Kickoff

## Stage Summary

Stage 109 adds `float` and `double` as first-class scalar types. In scope:
- `float` and `double` keyword tokens, type singletons (size/align 4/4 and 8/8)
- Floating-point literal scanning (decimal with fraction, exponent, f/F suffix; leading-dot form)
- `AST_FLOAT_LITERAL` AST node
- Local variable declaration and initialization with FP literals
- Global variable declaration (initialized → `.data`; uninitialized → `.bss`)
- Struct/union member declaration and assignment
- Implicit float→double widening (`cvtss2sd`)
- Per-translation-unit `.rodata` literal pool (`Lfc<N>: dd`/`dq`)

Not in scope: arithmetic, comparisons, function parameters, return values (Stages 110–112).

## Files to Change

| File | Change |
|------|--------|
| `include/token.h` | Add `TOKEN_FLOAT`, `TOKEN_DOUBLE`, `TOKEN_FLOAT_LITERAL`, `TOKEN_DOUBLE_LITERAL` |
| `include/type.h` | Add `TYPE_FLOAT`, `TYPE_DOUBLE`; prototypes `type_float()`, `type_double()` |
| `include/ast.h` | Add `AST_FLOAT_LITERAL` |
| `include/codegen.h` | Add `Vec fp_literals` to `CodeGen` |
| `src/type.c` | Singletons, accessors, `type_kind_name()`, `type_is_integer()` |
| `src/lexer.c` | Keyword recognition; FP literal scanning; `token_display_name()` |
| `src/parser.c` | Type-specifier, primary expression, all lookahead sets, file-scope initializer |
| `src/codegen.c` | Helpers, literal pool, load/store, widen, AST_VAR_REF, AST_DECLARATION, AST_ASSIGNMENT, globals, .rodata emission |
| `src/ast_pretty_printer.c` | `AST_FLOAT_LITERAL` case |
| `src/version.c` | Stage `01090000` |

## Key Design Decisions

**Single AST node type:** Use one `AST_FLOAT_LITERAL` for both float and double. The `decl_type` field (TYPE_FLOAT or TYPE_DOUBLE) distinguishes them. This resolves a spec copy-paste error where Task 3b listed two separate node types.

**Vec fp_literals in CodeGen:** The spec's Task 4a said "No new fields in CodeGen required," but Task 4e required a literal deduplication Vec. Resolve: add `Vec fp_literals` to the struct. This is the only workable approach.

**Deduplication by raw text:** "1.5f" (float) and "1.5" (double) are different keys even if their numeric values are equal (different NASM directives). Two occurrences of "1.5f" share one Lfc label.

**FP values travel in xmm0:** Load/store helpers use `movss` (float) and `movsd` (double). The integer result register `rax` is not involved.

**Implicit widening:** A float→double assignment emits `cvtss2sd xmm0, xmm0` between the load and store. No double→float narrowing in this stage.

**Suffix stripping for NASM:** NASM's `DD`/`DQ` directives accept decimal float values but not the C `f`/`F` suffix. Strip it before emission.
