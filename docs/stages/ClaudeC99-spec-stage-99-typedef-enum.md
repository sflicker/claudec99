# ClaudeC99 Stage 99 — `typedef enum` Completion

## Goal

Complete the `typedef enum` feature so that the idiomatic C99 patterns used in
real-world code compile correctly.  Two concrete gaps remain:

1. **Enum constant value expressions are restricted to integer/character
   literals.** Enumerators like `FLAG_READ = 1 << 0` or `LIMIT = BASE * 2`
   currently produce a parse error.  The fix: replace the literal-only check
   in `parse_enum_specifier` with a call to the existing
   `eval_case_const_expr`, and extend that evaluator to cover the full set
   of integer constant expression operators.

2. **`enum Tag` without a body fails when `Tag` has not been defined yet.**
   The common pattern of using `typedef enum Status Status;` before
   `enum Status { OK, ERR };` is rejected.  The fix: treat an undefined
   `enum Tag` as a forward-declared placeholder (analogous to how
   `struct Tag` works today) and return `type_int()` immediately.

This is a **language feature stage**.  No driver, lifecycle, or self-host
infrastructure changes.

---

## Background — what already works

Basic `typedef enum` is functional:

```c
typedef enum { RED, GREEN, BLUE } Color;   /* anonymous body → typedef */
typedef enum Color { RED, GREEN } Color;   /* named body → typedef    */
enum Status { OK, ERR };
typedef enum Status Status;               /* ref to defined tag       */
```

All three forms register the typedef name as a `TYPE_INT` alias and allow
the name to be used as a type specifier, in declarations, function
parameters/returns, sizeof, casts, and as `case`-label-compatible enum
constant expressions.

The two remaining gaps are:

### Gap 1 — Enumerator values are literals only

`parse_enum_specifier` (`src/parser.c`) accepts explicit enumerator values
only when they are bare integer or character literals:

```c
enum { A = 1, B = 2 };    /* works */
enum { A = 'x' };         /* works */
enum { A = 1 + 2 };       /* ERROR: enumerator value must be an int/char literal */
enum { A = 1 << 0 };      /* ERROR */
enum { A = 1, B = A + 1 };/* ERROR: A not recognised as a literal */
```

The existing `eval_case_const_expr` already handles integer/char literals
and enum-constant identifiers with `+`/`-`, but the enumerator parser
never calls it.

### Gap 2 — `enum Tag` forward references are rejected

`parse_enum_specifier` requires the tag to be in the `enum_tags` table
when no body is present:

```c
enum Status { OK, ERR };
typedef enum Status Status;     /* works — tag already defined */

typedef enum NodeType NodeType; /* ERROR: 'enum NodeType' is not defined */
enum NodeType { LEAF, BRANCH }; /* (would complete the forward ref) */
```

Struct and union tags support a forward-declaration placeholder today
(`include/parser.h`, `StructTag/UnionTag` with `type == NULL`).  Enum tags
do not have an equivalent mechanism.

---

## Grammar

No new tokens and no grammar productions change.  Informally, the
`<enumerator-constant>` grammar (C99 §6.7.2.2) is extended from

```
enumerator-constant ::= integer-literal | char-literal
```

to

```
enumerator-constant ::= <constant-integer-expression>
```

where `<constant-integer-expression>` is the same grammar as the `case`
label constant expression (see Task 1).

---

## Task 1 — Extend the integer constant expression evaluator

Rename the existing three helpers to reflect their new generality and add
the missing operators, maintaining precedence order (loosest last):

```
eval_const_primary(parser)
    INT_LITERAL | CHAR_LITERAL | IDENTIFIER(enum-const) | '(' expr ')'
    → current literal-only check + enum lookup (already in eval_case_const_primary)
    + new: consume '(' → call eval_const_expr → expect ')' → return value

eval_const_unary(parser)
    { '+' | '-' | '~' | '!' } eval_const_unary
    | eval_const_primary
    → already handles '+'/'-'; add '~' and '!'

eval_const_multiplicative(parser)          ← NEW level
    eval_const_unary { ('*' | '/' | '%') eval_const_unary }

eval_const_shift(parser)                   ← NEW level
    eval_const_multiplicative { ('<<' | '>>') eval_const_multiplicative }

eval_const_additive(parser)                ← was eval_case_const_expr
    eval_const_shift { ('+' | '-') eval_const_shift }

eval_const_bitwise_and(parser)             ← NEW level
    eval_const_additive { '&' eval_const_additive }

eval_const_bitwise_xor(parser)             ← NEW level
    eval_const_bitwise_and { '^' eval_const_bitwise_and }

eval_const_bitwise_or(parser)              ← NEW level
    eval_const_bitwise_xor { '|' eval_const_bitwise_xor }

eval_const_expr(parser)                    ← public name (was eval_case_const_expr)
    → calls eval_const_bitwise_or
```

The old names `eval_case_const_primary`, `eval_case_const_unary`, and
`eval_case_const_expr` are replaced.  The forward declaration that stage 97
added before `parse_initializer_element` must be updated to the new name.

**Division/modulo by zero** in a constant expression should call
`compile_error` (same behaviour as the preprocessor evaluator).

**Overflow** is silently wrapped (standard `long` arithmetic) — no change
from current behaviour.

**Error message update**: the existing message `"error: case label expression
is not an integer constant expression\n"` applies when an unexpected token
is encountered inside the evaluator.  For enumerator values, a different
message is more helpful:

```
"error: enumerator value is not an integer constant expression\n"
```

Pass the context via a `const char *context` parameter to `eval_const_expr`,
`eval_const_bitwise_or`, …, `eval_const_primary`:

```c
static long eval_const_expr(Parser *parser, const char *context);
```

Callers from `case`-label context pass `"case label expression"` and callers
from enumerator context pass `"enumerator value"` — so the error text is
always accurate.

---

## Task 2 — Use the extended evaluator in `parse_enum_specifier`

In `src/parser.c`, inside `parse_enum_specifier`, locate the enumerator
value assignment arm.  Currently it reads:

```c
if (parser->current.type == TOKEN_INT_LITERAL) {
    val = parser->current.long_value;
    parser->current = lexer_next_token(parser->lexer);
} else if (parser->current.type == TOKEN_CHAR_LITERAL) {
    val = (long)(unsigned char)parser->current.long_value;
    parser->current = lexer_next_token(parser->lexer);
} else {
    PARSER_ERROR(parser,
        "error: enumerator value must be an integer or character literal\n");
}
```

Replace it entirely with:

```c
val = eval_const_expr(parser, "enumerator value");
```

This single change enables all integer constant expression forms for
enumerator values, including previously-defined enum constants from the
same (or any preceding) enum.

No other changes to `parse_enum_specifier` are required.

---

## Task 3 — Allow forward-declared enum tags

In `src/parser.c`, inside `parse_enum_specifier`, the `enum Tag` (no body)
path currently errors when the tag is not yet in the `enum_tags` table:

```c
/* existing code (reference only — no body path) */
for (size_t i = 0; i < parser->enum_tags.len; i++) { ... }
/* falls through to: */
PARSER_ERROR(parser, "error: 'enum %s' is not defined\n", tag);
```

Replace the "not defined" error with a forward-declaration placeholder:

```c
/* Tag not found and no body — create a forward-declaration entry. */
EnumTag new_et;
new_et.tag        = tag;   /* pointer into lexer pool — no strdup needed */
new_et.is_defined = 0;
vec_push(&parser->enum_tags, &new_et);
/* Return type_int() — enum types are always int-sized. */
return type_int();
```

When a body is subsequently parsed for the same tag (the `{` branch),
the existing logic that searches for a prior entry and sets `is_defined = 1`
already handles completion.  No further change is needed there.

**Semantics note**: unlike struct/union incomplete types, an incomplete
`enum Tag` cannot be used for sizeof or as a value type — however, since
enum types are always `type_int()`, returning `type_int()` immediately is
safe and consistent.  Variables of type `enum Tag` declared before the body
compile correctly because their underlying type is always `int`.

---

## Out of scope — do NOT do these in this stage

- **Enum constant expressions that reference other enums defined later** —
  only already-registered constants are visible.  Out-of-order enum
  references remain an error.
- **Enum constant expressions in preprocessor `#if`** — the preprocessor
  has its own evaluator (`eval_cond_*`) which is separate from
  `eval_const_expr`.  No change there.
- **Designated union init for non-first members** — unrelated.
- **General integer constant expressions in object initializers**
  (`int x = 1 + 2;` at file scope) — a separate feature; the two
  evaluators are intentionally separate.
- **Enum type compatibility checking** — enum typed variables are treated
  as `int` throughout; stricter type checking is future work.
- **`typedef enum` at file scope without an alias name**
  (`typedef enum Foo { A, B };`) — currently accepted as a standalone
  no-op enum declaration; no change.
- **Comparison and logical operators (`==`, `!=`, `&&`, `||`) in
  `eval_const_expr`** — omitted intentionally; they are not needed for
  enumerator values or `case` labels in practice, and adding them would
  require a relational level and boolean-result semantics.  Can be added
  in a later stage if needed.

---

## Bootstrap caveats

The compiler must continue to self-host (C0→C1→C2).

`eval_const_expr` is a recursive descent function built entirely from
arithmetic.  No dynamic allocation and no new AST nodes are involved, so
there are no VLA or Vec reallocation concerns.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

### Valid tests

**Enumerator value — shift:**
```c
typedef enum {
    PERM_NONE  = 0,
    PERM_READ  = 1 << 0,
    PERM_WRITE = 1 << 1,
    PERM_EXEC  = 1 << 2
} Perm;
int main() {
    Perm p = PERM_READ | PERM_WRITE;
    return (int)p - 3;   /* expect 0 */
}
```

**Enumerator value — prior constant reference:**
```c
typedef enum {
    BASE  = 10,
    STEP  = BASE + 5,
    TOP   = STEP * 2
} Range;
int main() { return TOP - 30; }   /* expect 0 */
```

**Enumerator value — bitwise complement:**
```c
enum Mask { ALL = ~0, NONE = 0 };
int main() { return (ALL & 0xFF) == 0xFF ? 0 : 1; }   /* expect 0 */
```

**Enumerator value — parenthesized expression:**
```c
enum Sizes { SMALL = (4 * 8), LARGE = (4 * 64) };
int main() { return SMALL - 32; }   /* expect 0 */
```

**`case` label — shift expression (improvement via shared evaluator):**
```c
int main() {
    int x = 4;
    switch (x) {
        case 1 << 2: return 0;   /* 4 */
        default:     return 1;
    }
}
```

**`typedef enum` — forward reference then definition:**
```c
typedef enum Status Status;
enum Status { OK = 0, ERR = 1 };
Status check(int v) { return v ? ERR : OK; }
int main() { return check(1) - 1; }   /* expect 0 */
```

**`typedef enum` — forward reference with pointer:**
```c
typedef enum Color Color;
enum Color { RED = 1, GREEN = 2, BLUE = 3 };
int color_to_int(Color c) { return (int)c; }
int main() { return color_to_int(GREEN) - 2; }   /* expect 0 */
```

**`typedef enum` — forward reference across functions:**
```c
typedef enum Dir Dir;
enum Dir { NORTH = 0, SOUTH = 1, EAST = 2, WEST = 3 };
int opposite(Dir d) {
    switch (d) {
        case NORTH: return SOUTH;
        case SOUTH: return NORTH;
        case EAST:  return WEST;
        default:    return EAST;
    }
}
int main() { return opposite(NORTH) - 1; }   /* expect 0 */
```

**Already-working pattern (regression test):**
```c
typedef enum { A, B, C } Tri;
int main() { return B - 1; }   /* expect 0 */
```

### Invalid tests

**Enumerator value — non-constant expression:**
```c
int n = 5;
enum Bad { A = n };   /* error: enumerator value is not an integer constant expression */
int main() { return 0; }
```

**Enumerator value — division by zero:**
```c
enum Bad { A = 1 / 0 };   /* error: division by zero in constant expression */
int main() { return 0; }
```

### Print-AST tests

At minimum one print-AST test verifying that enum constants with
non-trivial values fold correctly to `INT_LITERAL` nodes in the AST
(e.g. `PERM_EXEC = 1 << 2` → `INT_LITERAL(4)` when expanded in
`parse_primary`).

---

## Implementation order

1. `src/parser.c` — rename `eval_case_const_primary` → `eval_const_primary`;
   add parenthesized-expression support to it.
2. `src/parser.c` — rename `eval_case_const_unary` → `eval_const_unary`;
   add `~` and `!` unary operators.
3. `src/parser.c` — add `eval_const_multiplicative` between unary and additive.
4. `src/parser.c` — rename former `eval_case_const_expr` (the additive loop)
   → `eval_const_additive`; change it to call `eval_const_multiplicative`.
5. `src/parser.c` — add `eval_const_shift`, `eval_const_bitwise_and`,
   `eval_const_bitwise_xor`, `eval_const_bitwise_or`.
6. `src/parser.c` — add `eval_const_expr` wrapper calling
   `eval_const_bitwise_or`; add the `const char *context` parameter
   throughout the chain.
7. `src/parser.c` — update the forward declaration used by
   `parse_initializer_element` to `eval_const_expr`.
8. `src/parser.c` — update the call site in `parse_case_label` (the
   `case` statement handler) to call `eval_const_expr(parser, "case label expression")`.
9. `src/parser.c` — update the call site in `parse_initializer_element`
   (stage 97 array index designator) to call
   `eval_const_expr(parser, "array designator index")`.
10. `src/parser.c` — update `parse_enum_specifier` to call
    `eval_const_expr(parser, "enumerator value")` in place of the
    literal-only check (Task 2).
11. `src/parser.c` — update `parse_enum_specifier` to accept an unknown
    `enum Tag` reference as a forward-declaration placeholder (Task 3).
12. `src/version.c` — bump `VERSION_STAGE` to `"00990000"`.
13. Tests — add valid, invalid, and print_ast tests.
14. Documentation (see below).

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — update "Through stage …" line; add `typedef enum` with
  constant expressions to the feature list; refresh test totals.
- **`docs/grammar.md`** — update the `<enumerator>` production to reference
  `<constant-integer-expression>` instead of `<integer-literal>`.
- Run the **`update-supplemental-docs`** skill to refresh the feature
  checklist, parse-tree, and status/features snapshots through stage 99.
- **`docs/self-compilation-report.md`** — record the stage-99 self-host run.
