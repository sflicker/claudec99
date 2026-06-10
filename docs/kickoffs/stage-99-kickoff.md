# Stage 99 Kickoff — `typedef enum` Completion

## Summary

Complete the `typedef enum` feature to support idiomatic C99 patterns used in real-world code. Two concrete gaps remain:

1. **Enumerator values are restricted to integer/character literals.** Expressions like `FLAG_READ = 1 << 0` or `LIMIT = BASE * 2` currently produce a parse error. Fix by extending the existing `eval_case_const_expr` constant-expression evaluator to cover the full set of integer constant expression operators, then call it from enumerator-value parsing.

2. **`enum Tag` without a body fails when `Tag` is undefined.** The common pattern `typedef enum Status Status;` before `enum Status { OK, ERR };` is rejected. Fix by treating an undefined `enum Tag` as a forward-declared placeholder (like struct tags do today) and returning `type_int()` immediately.

This is a **language feature stage** touching the parser only. No tokenizer, AST, or code generation changes. The self-host cycle must continue to pass.

---

## Tokenizer Changes

None. No new tokens required.

---

## Parser Changes

**Task 1: Extend the integer constant expression evaluator**

Rename and extend the existing `eval_case_const_expr` chain to `eval_const_expr`, adding missing operators:

1. Rename `eval_case_const_primary` → `eval_const_primary`; add parenthesized-expression support `'(' eval_const_expr ')'`.

2. Rename `eval_case_const_unary` → `eval_const_unary`; add unary operators `~` (bitwise NOT) and `!` (logical NOT).

3. Add `eval_const_multiplicative` (new precedence level): `*`, `/`, `%`.
   - Detect and report division/modulo by zero with `compile_error`.

4. Rename former `eval_case_const_expr` (the additive loop) → `eval_const_additive`; change it to call `eval_const_multiplicative` instead of `eval_const_unary`.

5. Add four new levels in operator-precedence order:
   - `eval_const_shift` — `<<`, `>>`
   - `eval_const_bitwise_and` — `&`
   - `eval_const_bitwise_xor` — `^`
   - `eval_const_bitwise_or` — `|`

6. Add public wrapper `eval_const_expr(Parser *parser, const char *context)` calling `eval_const_bitwise_or`. Pass `context` through the entire chain; use it in error messages:
   - When an unexpected token is encountered, emit: `"error: <context> is not an integer constant expression\n"`
   - When division/modulo by zero occurs, emit: `"error: division by zero in constant expression\n"` (context-independent).

7. Update the forward declaration before `parse_initializer_element` (stage 97) from `eval_case_const_expr` to the new signature.

8. Update the `parse_case_label` call site to invoke `eval_const_expr(parser, "case label expression")`.

9. Update the `parse_initializer_element` call site (array designator index) to invoke `eval_const_expr(parser, "array designator index")`.

**Task 2: Use the extended evaluator in `parse_enum_specifier`**

In `parse_enum_specifier`, replace the literal-only enumerator-value check with:

```c
val = eval_const_expr(parser, "enumerator value");
```

This enables all integer constant expression forms, including references to previously-defined enum constants.

**Task 3: Allow forward-declared enum tags**

In `parse_enum_specifier`, when parsing `enum Tag` (no body) and the tag is not found in `enum_tags`, replace the "not defined" error with a forward-declaration entry:

```c
EnumTag new_et;
new_et.tag        = tag;   /* pointer into lexer pool — no strdup needed */
new_et.is_defined = 0;
vec_push(&parser->enum_tags, &new_et);
return type_int();
```

The existing logic that searches for a prior entry and sets `is_defined = 1` when a body is parsed handles completion automatically.

---

## AST Changes

None.

---

## Code Generation Changes

None.

---

## Test Plan

**Valid tests (9):**
- Enumerator value: shift operators (`1 << 0`)
- Enumerator value: prior constant reference (`BASE + 5`, `STEP * 2`)
- Enumerator value: bitwise complement (`~0`)
- Enumerator value: parenthesized expression (`(4 * 8)`)
- Case label: shift expression (`case 1 << 2:`)
- `typedef enum` — forward reference then definition
- `typedef enum` — forward reference with pointer
- `typedef enum` — forward reference across functions
- Regression: already-working pattern (anonymous enum)

**Invalid tests (2):**
- Enumerator value: non-constant expression (variable reference)
- Enumerator value: division by zero

**`print_ast` tests (1):**
- Enum constants with non-trivial values fold correctly to `INT_LITERAL` nodes (e.g. `PERM_EXEC = 1 << 2` → `INT_LITERAL(4)`)

**Existing test updates (2):**
- `test_invalid_enum_expr_value__enumerator_value_must_be.c` — change from literal-only to variable reference (remains invalid)
- `test_invalid_enum_missing_tag__is_not_defined.c` — remove or update (forward ref now accepted)

---

## Implementation Order

1. `src/parser.c`:
   a. Rename `eval_case_const_primary` → `eval_const_primary`; add `'(' eval_const_expr ')'` support
   b. Rename `eval_case_const_unary` → `eval_const_unary`; add `~` and `!` operators
   c. Add `eval_const_multiplicative`
   d. Rename additive loop → `eval_const_additive`; call `eval_const_multiplicative`
   e. Add `eval_const_shift`, `eval_const_bitwise_and`, `eval_const_bitwise_xor`, `eval_const_bitwise_or`
   f. Add `eval_const_expr` wrapper with `const char *context` parameter throughout
   g. Update forward declaration used by `parse_initializer_element`
   h. Update `parse_case_label` call site
   i. Update `parse_initializer_element` call site
   j. Update `parse_enum_specifier` to call `eval_const_expr(parser, "enumerator value")`
   k. Update `parse_enum_specifier` to forward-declare unknown enum tags

2. `src/version.c` — bump `VERSION_STAGE` to `"00990000"`

3. Tests — add valid (9), invalid (2), `print_ast` (1); update existing (2)

4. Documentation: README.md, docs/grammar.md, supplemental snapshots, self-host report

---

## Key Decisions

- **Context parameter:** Passed through the entire evaluator chain to provide context-specific error messages.

- **Division/modulo by zero:** Reported with a context-independent error message (same as preprocessor).

- **Overflow:** Silently wrapped using standard `long` arithmetic (no change from current behavior).

- **Forward-declaration semantics:** Like struct tags, enum forward declarations create a placeholder with `is_defined = 0`. When the body is parsed, the existing lookup logic sets `is_defined = 1`.

- **Operator precedence:** Strictly follows C99 integer constant expression rules (parentheses, unary, multiplicative, shift, bitwise-and, bitwise-xor, bitwise-or, additive).

---

## Ambiguities Resolved

- **Undefined enum tag:** Forward-declare with a placeholder entry (tag without body), allowing later completion.

- **Enumerator referencing prior enums:** The existing enum-constant lookup in `eval_const_primary` (renamed from `eval_case_const_primary`) already handles this.

- **Error messages:** Context parameter ensures accuracy:
  - `"error: case label expression is not an integer constant expression\n"`
  - `"error: array designator index is not an integer constant expression\n"`
  - `"error: enumerator value is not an integer constant expression\n"`

- **Overflow behavior:** No change from current behavior (wrapping arithmetic).

---

## Bootstrap Notes

The extended evaluator is a pure recursive descent function using no dynamic allocation, VLAs, or new AST nodes. Self-host (C0→C1→C2) is expected to pass unchanged.

Run `./build.sh --mode=self-host` before closing the stage.

