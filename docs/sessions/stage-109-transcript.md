# stage-109 Transcript: Float and Double Types, Literals, and Stack Variables

## Summary

Stage 109 introduces `float` and `double` as first-class types to the ClaudeC99 compiler. Both types support variable declaration, initialization, assignment, struct/union membership, and global storage. Floating-point literals are lexed using a generalized digit-scanning path that detects decimal points, exponents, and optional suffixes, then stored in a deduplicated `.rodata` pool. Type-system changes include two new singleton types with correct sizes and alignments. The code generator implements SSE-based load/store operations (`movss`/`movsd`), automatic stack/global allocation, and implicit float-to-double widening via `cvtss2sd`. Arithmetic operations, comparisons, and function parameters/return values are explicitly deferred to Stages 110–112.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_FLOAT`, `TOKEN_DOUBLE`, `TOKEN_FLOAT_LITERAL`, `TOKEN_DOUBLE_LITERAL` to token types
- Extended lexer keyword-recognition block to detect `float` and `double` keywords and set appropriate token type
- Extended digit-sequence scanner in `lexer_next_token` to distinguish FP literals from integers:
  1. After scanning initial digit sequence, check for decimal point (`.`) or exponent (`e`/`E`)
  2. If present, continue scanning fractional digits or process exponent (`[+/-]digits`)
  3. After all numeric content, check for `f`/`F` suffix
  4. Return `TOKEN_FLOAT_LITERAL` if `f`/`F` present, `TOKEN_DOUBLE_LITERAL` if `.` or `e`/`E` present without suffix
  5. Preserve raw literal text (including suffix) in `token.value` for NASM processing
- Added special case for leading-dot literals (`.5`, `.123e2`) detected before main digit sequence
- Updated `token_display_name()` to map all four new tokens to human-readable names

### Step 2: Type System

- Added `TYPE_FLOAT` and `TYPE_DOUBLE` to `TypeKind` enum
- Defined singleton accessors `type_float()` and `type_double()` returning static `Type` instances
- Configured size and alignment: `TYPE_FLOAT` (4 bytes, 4-byte align), `TYPE_DOUBLE` (8 bytes, 8-byte align)
- Added `type_is_fp()` predicate helper to test if a type is floating-point
- Updated `type_kind_name()` to return `"float"` and `"double"` for display
- Updated `type_is_integer()` to return false for FP types

### Step 3: AST

- Added `AST_FLOAT_LITERAL` node type to `ASTNodeType` enum
- Parser sets `node->decl_type` to either `TYPE_FLOAT` or `TYPE_DOUBLE` to distinguish within the same node type
- `node->result_type` set to corresponding singleton (`type_float()` or `type_double()`)
- `node->value` holds raw literal text (e.g., `"1.5f"`, `"3.14"`)

### Step 4: Parser

- Extended `parse_type_specifier()` to recognize `TOKEN_FLOAT` and `TOKEN_DOUBLE`, returning `type_float()` or `type_double()` respectively
- Updated `parse_primary()` to handle `TOKEN_FLOAT_LITERAL` and `TOKEN_DOUBLE_LITERAL` tokens:
  - Create `AST_FLOAT_LITERAL` node (regardless of float vs. double; type distinguished via `decl_type`)
  - Set `result_type` to match the literal's actual type
- Added `TOKEN_FLOAT` and `TOKEN_DOUBLE` to all type-start lookahead sets:
  - Postfix compound-literal check
  - `sizeof` type-name check
  - Cast expression type-name check
  - For-loop initializer declaration check
  - Block-scope statement dispatch
  - File-scope declaration dispatch
  - `eval_const_primary` sizeof argument check
- Implemented implicit float-to-double widening in assignment and declaration-initializer paths:
  - Permitted `float` rvalue → `double` lvalue assignment
  - Codegen later emits `cvtss2sd` conversion
  - Reverse (double → float) rejected or left to explicit cast
- Added FP-specific branch for file-scope initializers: parser validates the initializer is `AST_FLOAT_LITERAL`

### Step 5: Code Generator

- Defined `FpLiteral` typedef: `{const char *raw_text; int label_index; int is_double;}`
- Added `Vec fp_literals` field to `CodeGen` struct to track unique FP literals with `.rodata` labels
- Implemented `codegen_intern_fp_literal()` to search the literal pool and return label index (reuse existing or append new)
- Added helper functions:
  - `emit_load_fp_local()`: emit `movss xmm0, [rbp - offset]` or `movsd xmm0, [rbp - offset]` for float/double
  - `emit_store_fp_local()`: emit `movss [rbp - offset], xmm0` or `movsd [rbp - offset], xmm0`
  - `emit_load_fp_global()`: emit RIP-relative `movss xmm0, [rel Lglobal_name]` or `movsd`
  - `emit_store_fp_global()`: emit RIP-relative store
  - `emit_fp_widen_if_needed()`: emit `cvtss2sd xmm0, xmm0` if lhs is double and rhs is float
  - `type_kind_bytes()`: return size for float/double (4 or 8)
- Updated `AST_VAR_REF` codegen: FP local loads via `emit_load_fp_local()`; FP global loads via `emit_load_fp_global()`
- Updated `AST_FLOAT_LITERAL` codegen: emit RIP-relative load from FP literal pool via `codegen_intern_fp_literal()`
- Updated `AST_DECLARATION` codegen: FP local scalar path allocates stack slot and emits store-from-xmm0 if initializer present
- Updated `AST_ASSIGNMENT` codegen: FP paths for local, global, member-dot, and member-arrow with implicit widening check
- Added FP global initialization in `codegen_add_global()`: store initializer to `.data` or `.bss` entry
- Added `data_init_directive()` helper to format `.data` entries for float/double (using NASM `dd`/`dq` with decimal)
- Added `bss_res_directive()` helper to emit `.bss` `resq`/`resd` allocation
- Updated `codegen_emit_data()` to handle FP global emission
- Implemented `codegen_emit_fp_literals()` to emit entire `.rodata` FP literal pool:
  - Strip `f`/`F` suffix from raw text before emitting to NASM (e.g., `1.5f` → `1.5`)
  - Emit labels `Lfc_0`, `Lfc_1`, etc.
  - Use `DD` directive for 32-bit float, `DQ` for 64-bit double
  - NASM parses the decimal value and converts to IEEE 754
- Called `codegen_emit_fp_literals()` from `codegen_translation_unit()` after all other sections

### Step 6: AST Pretty Printer

- Added case for `AST_FLOAT_LITERAL` in pretty-printer switch
- Output format: `FloatLiteral: <value>` (using `node->value` raw text)

### Step 7: Version Bump

- Updated `VERSION_STAGE` in `src/version.c` to `"01090000"`

### Step 8: Tests

Created 6 new test cases in `test/valid/`:

1. **test_float_declare__0.c**: Declare and initialize a local `float` variable with `1.0f` literal
2. **test_double_declare__0.c**: Declare and initialize a local `double` variable with `3.14` literal
3. **test_float_copy__0.c**: Copy a `float` local to another `float` local variable
4. **test_double_widen__0.c**: Implicit `float` to `double` widening assignment
5. **test_float_struct_member__0.c**: Struct with two `float` members, assigned via member address with `movss`
6. **test_double_global__0.c**: Global `double` variable initialized and loaded into local

All tests verify correct behavior via exit code 0 (implicit through `main` return).

## Final Results

**Build**: Successful; all object files link cleanly.

**Test Results**: All 1627 tests pass (1621 pre-existing + 6 new).

**Regression**: Zero failures; no existing tests affected.

**Self-Hosting**: C0→C1→C2 bootstrap cycle completed cleanly.

| Step | Version | Tests |
|------|---------|-------|
| C0 (GCC build) | `00.02.01090000.00867` | 1627/1627 |
| C1 (C0 bootstrap) | `00.02.01090000.00868` | 1627/1627 |
| C2 (C1 bootstrap) | `00.02.01090000.00869` | 1627/1627 |

The compiler's own source contains no `float` or `double` usage, so all new code paths remain inert during self-compilation. Compiler is now a fixed point (C1 and C2 behavior-identical).

## Session Flow

1. Read stage spec and supporting templates
2. Analyzed type system and code-generation infrastructure
3. Planned tokenizer changes for FP keyword and literal recognition
4. Implemented token layer (4 new tokens, keyword/literal scanning)
5. Designed and implemented type system (singletons, accessors, helpers)
6. Added AST node type and parser recognition (all lookahead sets updated)
7. Implemented code generator (register convention, load/store, literal pool, implicit widening)
8. Updated AST pretty printer
9. Bumped version
10. Created 6 test cases
11. Ran full test suite (all 1627 pass)
12. Executed self-host C0→C1→C2 cycle (all stages pass with 1627/1627)
13. Generated documentation artifacts (kickoff, milestone, transcript, grammar, checklist, README)

## Notes

- **FP Literal Deduplication**: Conservative approach—textually identical literals share one label; textually distinct literals with identical IEEE 754 values get separate labels (correct per spec).
- **Implicit Widening Only**: float→double widening is automatic; all other conversions (including double→float) require explicit cast.
- **Compiler Self-Reference**: The compiler's own codebase uses no floating-point types, so Stage 109 implementation is orthogonal to self-hosting stability.
- **Deferred Features**: FP arithmetic (`+`, `-`, `*`, `/`, unary `-`), comparisons (`<`, `==`, etc.), and function parameters/return values are explicitly deferred to Stages 110–112.
