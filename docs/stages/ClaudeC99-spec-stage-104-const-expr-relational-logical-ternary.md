# ClaudeC99 Stage 104 — Complete Constant-Expression Evaluator

## Goal

Extend both compile-time constant-expression evaluators to cover the full
C99 integer constant expression operator set:

```c
/* Relational operators */
enum { POSITIVE = 1 > 0 };               /* relational:  1   */
static int eq   = (sizeof(int) == 4);    /* equality:    1   */
int SMALL = 255 < 256;                   /* file-scope:  1   */

/* Logical operators */
enum { BOTH = (FLAG_A && FLAG_B) };      /* logical AND: 1   */
static int any = (0 || DEFAULT_VAL);     /* logical OR: DEFAULT_VAL */

/* Conditional / ternary */
enum { LIMIT = DEBUG_MODE ? 10 : 100 };  /* ternary:     100 */
static int sz = sizeof(int)==4 ? 32 : 64; /* conditional  */
```

These patterns — valid C99 — are currently rejected by both evaluators with
`"error: %s is not an integer constant expression"` or
`"error: initializer for block-scope static '%s' must be a constant expression"`.

This stage also fixes a **pre-existing precedence bug** in the parser evaluator
where `eval_const_additive` and `eval_const_shift` are inverted, causing
`3 + 1 << 2` to evaluate as `3 + (1 << 2) = 7` instead of the correct
`(3 + 1) << 2 = 16`.

This is a **parser and codegen stage**.  No new tokens, AST node types, or
grammar rules are added.

---

## Background — current state

Two independent constant-expression evaluators exist in the compiler:

**1. `eval_const_expr` (token-stream evaluator, `src/parser.c`)**

Used by: `case` label values, `enum` enumerator values, array designator
indices (`[expr]`), and file-scope integer variable initializers (stage 100).

The evaluator is a recursive-descent chain with nine functions (loosest
to tightest binding):

```
eval_const_expr
  → eval_const_bitwise_or   (|)
    → eval_const_bitwise_xor  (^)
      → eval_const_bitwise_and  (&)
        → eval_const_additive   (+, -)   ← BUG: should call eval_const_shift
          → eval_const_shift    (<<, >>)  ← BUG: should call eval_const_additive
            → eval_const_multiplicative  (*, /, %)
              → eval_const_unary  (+, -, ~, !)
                → eval_const_primary  (literal, enum-const, sizeof, '(')
```

Missing operator levels: relational (`< <= > >=`), equality (`== !=`),
logical-AND (`&&`), logical-OR (`||`), and conditional (`? :`).

**2. `eval_const_init` (AST evaluator, `src/codegen.c`)**

Used by: the SC_STATIC scalar arm of `codegen_statement` (block-scope static
scalar initializers, added in stage 103).

The evaluator walks an already-parsed AST and handles: `AST_INT_LITERAL`,
`AST_CHAR_LITERAL`, `AST_SIZEOF_TYPE`, `AST_UNARY_OP` (`+`, `-`, `~`, `!`),
and `AST_BINARY_OP` (`+`, `-`, `*`, `/`, `%`, `<<`, `>>`, `&`, `^`, `|`).

Missing: `AST_BINARY_OP` with operators `<`, `<=`, `>`, `>=`, `==`, `!=`,
`&&`, `||`, and `AST_CONDITIONAL_EXPR`.

(The precedence bug does **not** affect `eval_const_init` because it operates
on an AST already built by the correct `parse_shift`/`parse_additive` chain.)

---

## The additive / shift precedence bug

In C99, `<<` and `>>` have **lower** precedence than `+` and `-` (shift is
level 5, additive is level 6 out of 15, where higher = tighter in the
standard precedence table).  Operationally:

```c
3 + 1 << 2    /* C99: (3 + 1) << 2 = 16   (additive is tighter than shift) */
```

The regular expression parser is correct:

```c
static ASTNode *parse_shift(Parser *parser) {
    ASTNode *left = parse_additive(parser);   /* additive is inner = tighter */
    ...
```

The constant-expression evaluator has the relationship inverted:

```c
static long eval_const_additive(Parser *parser, const char *context) {
    long val = eval_const_shift(parser, context);  /* BUG: shift is inner = tighter */
    ...
```

This means `3 + 1 << 2` in a constant-expression context (case label, enum
value, file-scope initializer, block-scope static) evaluates as
`3 + (1 << 2) = 7` instead of `16`.  The bug is silent in the test suite
because no existing test mixes `+` and `<<` without parentheses.

The fix: swap the call order so `eval_const_shift` calls `eval_const_additive`
and `eval_const_additive` calls `eval_const_multiplicative`.

---

## Grammar

No new tokens or productions.  Informally, the complete grammar for both
evaluators after this stage (loosest to tightest):

```
const_expr         := const_conditional
const_conditional  := const_logical_or ( '?' const_expr ':' const_conditional )?
const_logical_or   := const_logical_and ( '||' const_logical_and )*
const_logical_and  := const_bitwise_or  ( '&&' const_bitwise_or  )*
const_bitwise_or   := const_bitwise_xor ( '|'  const_bitwise_xor )*
const_bitwise_xor  := const_bitwise_and ( '^'  const_bitwise_and )*
const_bitwise_and  := const_equality    ( '&'  const_equality    )*
const_equality     := const_relational  ( ('==' | '!=') const_relational )*
const_relational   := const_shift       ( ('<' | '<=' | '>' | '>=') const_shift )*
const_shift        := const_additive    ( ('<<' | '>>') const_additive )*
const_additive     := const_mult        ( ('+' | '-') const_mult )*
const_mult         := const_unary       ( ('*' | '/' | '%') const_unary )*
const_unary        := ('+' | '-' | '~' | '!') const_unary | const_primary
const_primary      := INT_LITERAL | CHAR_LITERAL | enum-const-IDENTIFIER
                    | sizeof '(' type-name ')'
                    | '(' const_expr ')'
```

(Stages 99–100 implemented `const_expr` through `const_bitwise_or`.
Stage 104 adds the six missing levels above and fixes the additive/shift swap.)

---

## Task 1 — Fix additive/shift and insert relational/equality in `src/parser.c`

### 1a. Fix `eval_const_additive` and `eval_const_shift`

**Before (`eval_const_additive` calls `eval_const_shift` — wrong):**

```c
static long eval_const_additive(Parser *parser, const char *context) {
    long val = eval_const_shift(parser, context);
    while (parser->current.type == TOKEN_PLUS ||
           parser->current.type == TOKEN_MINUS) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_shift(parser, context);
        if (op == TOKEN_PLUS) val += rhs;
        else                  val -= rhs;
    }
    return val;
}

static long eval_const_shift(Parser *parser, const char *context) {
    long val = eval_const_multiplicative(parser, context);
    while (parser->current.type == TOKEN_LEFT_SHIFT ||
           parser->current.type == TOKEN_RIGHT_SHIFT) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_multiplicative(parser, context);
        if (op == TOKEN_LEFT_SHIFT) val <<= rhs;
        else                        val >>= rhs;
    }
    return val;
}
```

**After (`eval_const_shift` calls `eval_const_additive` — correct):**

```c
static long eval_const_additive(Parser *parser, const char *context) {
    long val = eval_const_multiplicative(parser, context);
    while (parser->current.type == TOKEN_PLUS ||
           parser->current.type == TOKEN_MINUS) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_multiplicative(parser, context);
        if (op == TOKEN_PLUS) val += rhs;
        else                  val -= rhs;
    }
    return val;
}

static long eval_const_shift(Parser *parser, const char *context) {
    long val = eval_const_additive(parser, context);
    while (parser->current.type == TOKEN_LEFT_SHIFT ||
           parser->current.type == TOKEN_RIGHT_SHIFT) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_additive(parser, context);
        if (op == TOKEN_LEFT_SHIFT) val <<= rhs;
        else                        val >>= rhs;
    }
    return val;
}
```

Order in source file: `eval_const_additive` must be defined **before**
`eval_const_shift` because `eval_const_shift` now calls it.  The current
file has them in `eval_const_additive` → `eval_const_shift` order, which is
fine (no forward declaration needed; it is already present for
`eval_const_expr`).

### 1b. Add `eval_const_relational` and `eval_const_equality`

Insert these two new functions between `eval_const_shift` and
`eval_const_bitwise_and` in the source file:

```c
static long eval_const_relational(Parser *parser, const char *context) {
    long val = eval_const_shift(parser, context);
    while (parser->current.type == TOKEN_LT ||
           parser->current.type == TOKEN_LE ||
           parser->current.type == TOKEN_GT ||
           parser->current.type == TOKEN_GE) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_shift(parser, context);
        if      (op == TOKEN_LT) val = val <  rhs;
        else if (op == TOKEN_LE) val = val <= rhs;
        else if (op == TOKEN_GT) val = val >  rhs;
        else                     val = val >= rhs;
    }
    return val;
}

static long eval_const_equality(Parser *parser, const char *context) {
    long val = eval_const_relational(parser, context);
    while (parser->current.type == TOKEN_EQ ||
           parser->current.type == TOKEN_NE) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_relational(parser, context);
        if (op == TOKEN_EQ) val = val == rhs;
        else                val = val != rhs;
    }
    return val;
}
```

### 1c. Update `eval_const_bitwise_and` to call `eval_const_equality`

**Before:**
```c
static long eval_const_bitwise_and(Parser *parser, const char *context) {
    long val = eval_const_additive(parser, context);
```

**After:**
```c
static long eval_const_bitwise_and(Parser *parser, const char *context) {
    long val = eval_const_equality(parser, context);
```

---

## Task 2 — Add logical-AND, logical-OR, and conditional in `src/parser.c`

Insert these three functions between `eval_const_bitwise_or` and
`eval_const_expr` (currently `eval_const_expr` just calls
`eval_const_bitwise_or`):

```c
static long eval_const_logical_and(Parser *parser, const char *context) {
    long val = eval_const_bitwise_or(parser, context);
    while (parser->current.type == TOKEN_AND_AND) {
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_bitwise_or(parser, context);
        val = (val && rhs) ? 1 : 0;
    }
    return val;
}

static long eval_const_logical_or(Parser *parser, const char *context) {
    long val = eval_const_logical_and(parser, context);
    while (parser->current.type == TOKEN_OR_OR) {
        parser->current = lexer_next_token(parser->lexer);
        long rhs = eval_const_logical_and(parser, context);
        val = (val || rhs) ? 1 : 0;
    }
    return val;
}

static long eval_const_conditional(Parser *parser, const char *context) {
    long cond = eval_const_logical_or(parser, context);
    if (parser->current.type != TOKEN_QUESTION)
        return cond;
    parser->current = lexer_next_token(parser->lexer); /* consume '?' */
    long true_val  = eval_const_expr(parser, context);
    parser_expect(parser, TOKEN_COLON);
    long false_val = eval_const_conditional(parser, context);
    return cond ? true_val : false_val;
}
```

**Notes:**

- `eval_const_logical_and` and `eval_const_logical_or` always evaluate both
  operands (both are constants, so no short-circuit is needed for
  correctness).  The result is normalized to `0` or `1`.
- `eval_const_conditional` is right-associative (calls `eval_const_conditional`
  recursively for the false branch), matching `parse_conditional`.
- The true branch uses `eval_const_expr` (which allows the full expression
  grammar including another ternary), and the false branch recurses through
  `eval_const_conditional`, matching standard C ternary associativity.

### Update `eval_const_expr` to call `eval_const_conditional`

**Before:**
```c
static long eval_const_expr(Parser *parser, const char *context) {
    return eval_const_bitwise_or(parser, context);
}
```

**After:**
```c
static long eval_const_expr(Parser *parser, const char *context) {
    return eval_const_conditional(parser, context);
}
```

### Update the grammar comment block

Update the comment at the top of `eval_const_primary` (around line 2565):

```c
/*
 * Stage 99–104: Compile-time integer constant expression evaluator.
 *
 * Grammar (loosest to tightest):
 *   const_expr         := const_conditional
 *   const_conditional  := const_logical_or ( '?' const_expr ':' const_conditional )?
 *   const_logical_or   := const_logical_and ( '||' const_logical_and )*
 *   const_logical_and  := const_bitwise_or  ( '&&' const_bitwise_or  )*
 *   const_bitwise_or   := const_bitwise_xor ( '|'  const_bitwise_xor )*
 *   const_bitwise_xor  := const_bitwise_and ( '^'  const_bitwise_and )*
 *   const_bitwise_and  := const_equality    ( '&'  const_equality    )*
 *   const_equality     := const_relational  ( ('==' | '!=') const_relational )*
 *   const_relational   := const_shift       ( ('<' | '<=' | '>' | '>=') const_shift )*
 *   const_shift        := const_additive    ( ('<<' | '>>') const_additive )*
 *   const_additive     := const_mult        ( ('+' | '-') const_mult )*
 *   const_mult         := const_unary       ( ('*' | '/' | '%') const_unary )*
 *   const_unary        := ('+' | '-' | '~' | '!') const_unary | const_primary
 *   const_primary      := INT_LITERAL | CHAR_LITERAL | enum-const-IDENTIFIER
 *                       | sizeof '(' type-name ')'
 *                       | '(' const_expr ')'
 *
 * context is "case label expression", "enumerator value",
 * "array designator index", or "file-scope initializer".
 * Calls PARSER_ERROR (does not return) if a non-constant operand is found.
 */
```

---

## Task 3 — Extend `eval_const_init` in `src/codegen.c`

### 3a. Add relational and equality operators to `AST_BINARY_OP` block

In `eval_const_init`, inside the `AST_BINARY_OP && node->child_count == 2`
block, add the six new operator comparisons **before** the final
`compile_error` fallthrough.

The existing block ends with `if (strcmp(node->value, "|") == 0)`.  Add
immediately after:

```c
        if (strcmp(node->value, "<")  == 0) return lhs <  rhs;
        if (strcmp(node->value, "<=") == 0) return lhs <= rhs;
        if (strcmp(node->value, ">")  == 0) return lhs >  rhs;
        if (strcmp(node->value, ">=") == 0) return lhs >= rhs;
        if (strcmp(node->value, "==") == 0) return lhs == rhs;
        if (strcmp(node->value, "!=") == 0) return lhs != rhs;
        if (strcmp(node->value, "&&") == 0) return (lhs && rhs) ? 1 : 0;
        if (strcmp(node->value, "||") == 0) return (lhs || rhs) ? 1 : 0;
```

**Note on `&&` and `||` evaluation:** Both operands are already evaluated
eagerly before the comparisons run.  This is correct because both sides of a
`&&` or `||` in a constant expression are themselves constant expressions —
eager evaluation cannot cause side effects.  If the right operand would
divide by zero it would already have been caught in its own recursive call.

### 3b. Add `AST_CONDITIONAL_EXPR` case

Add a new branch **before** the final `compile_error`, after the
`AST_BINARY_OP` block:

```c
    if (node->type == AST_CONDITIONAL_EXPR && node->child_count == 3) {
        long cond = eval_const_init(node->children[0], varname);
        if (cond)
            return eval_const_init(node->children[1], varname);
        else
            return eval_const_init(node->children[2], varname);
    }
```

`AST_CONDITIONAL_EXPR` nodes have three children: `[0]` = condition,
`[1]` = true branch, `[2]` = false branch (set by `parse_conditional` in
`src/parser.c`).

Unlike `&&`/`||`, only one branch of `?:` is evaluated (true short-circuit):
if the condition is nonzero, `children[2]` is never evaluated.  This matters
when the unused branch contains a construct that would call `compile_error`
(e.g., a division by zero or a non-constant reference).

---

## Task 4 — `src/version.c`

Bump `VERSION_STAGE` to `"01040000"`.

---

## Implementation order

1. `src/parser.c` — swap `eval_const_additive`/`eval_const_shift` call order
   (Task 1a).
2. `src/parser.c` — add `eval_const_relational` and `eval_const_equality`; update
   `eval_const_bitwise_and` to call `eval_const_equality` (Task 1b–1c).
3. `src/parser.c` — add `eval_const_logical_and`, `eval_const_logical_or`,
   `eval_const_conditional`; update `eval_const_expr` (Task 2); update grammar
   comment.
4. `src/codegen.c` — add new operators to `eval_const_init` (Task 3).
5. `src/version.c` — bump stage (Task 4).
6. Tests.
7. Documentation.

---

## Out of scope — do NOT do these in this stage

- **Short-circuit evaluation of `&&`/`||` in `eval_const_init`**: not needed
  because constant expressions cannot have side effects; eager evaluation is
  correct and simpler.
- **Relational/equality in `#if`/`#elif` preprocessor expressions**: those
  already support `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||` (implemented
  in the preprocessor evaluator in stage 81).  The parser evaluator is a
  separate code path.
- **`sizeof(expr)` (as opposed to `sizeof(type)`) in constant expressions**:
  already excluded by `eval_const_primary`; no change.
- **Comma operator in constant expressions**: `a, b` is not a C99 constant
  expression and is not added.
- **Floating-point constant expressions**: out of scope project-wide.
- **`const`-qualified variables as constant references**: C99 §6.6 does not
  admit non-enum, non-literal identifiers; `eval_const_primary` and
  `eval_const_init` correctly reject them.
- **Compound literal and cast expressions in constant contexts**: out of scope.

---

## Bootstrap caveats

The compiler's own source uses `eval_const_expr` for `case` labels and `enum`
values only; none of those uses mix `+`/`-` with `<<`/`>>` without
parentheses, so the additive/shift fix does not change the evaluated value of
any expression in the compiler's own source.  The new operators (`<`, `==`,
`&&`, `||`, `?:`) do not appear in the compiler's own constant expressions.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

### Valid — parser evaluator (case / enum / file-scope)

**Relational in enum:**
```c
enum { POSITIVE = 1 > 0, NEGATIVE = 0 > 1 };
int main(void) { return POSITIVE - 1 + NEGATIVE; }  /* expect 0 */
```

**Equality in enum:**
```c
enum { SAME = 5 == 5, DIFF = 5 == 6 };
int main(void) { return SAME - 1 + DIFF; }  /* expect 0 */
```

**Logical AND in enum:**
```c
enum { A = 1 && 1, B = 1 && 0, C = 0 && 1 };
int main(void) { return A - 1 + B + C; }   /* expect 0 */
```

**Logical OR in enum:**
```c
enum { A = 1 || 0, B = 0 || 1, C = 0 || 0 };
int main(void) { return A - 1 + B - 1 + C; }  /* expect 0 */
```

**Ternary in enum:**
```c
enum { X = 1 ? 42 : 0, Y = 0 ? 0 : 7 };
int main(void) { return X - 42 + Y - 7; }  /* expect 0 */
```

**Additive/shift precedence fix (file-scope):**
```c
int x = 3 + 1 << 2;   /* (3+1)<<2 = 16; was 7 with old bug */
int main(void) { return x - 16; }   /* expect 0 */
```

**Relational in case label:**
```c
int main(void) {
    int n = 1;
    switch (n) {
    case (2 > 1): return 0;   /* case 1 */
    default:      return 1;
    }
}  /* expect 0 */
```

**Ternary in file-scope initializer:**
```c
int big = sizeof(long) == 8 ? 64 : 32;
int main(void) { return big - 64; }   /* expect 0 on LP64 */
```

### Valid — codegen evaluator (block-scope static)

**Relational:**
```c
int main(void) {
    static int x = 3 > 2;
    return x - 1;   /* expect 0 */
}
```

**Equality:**
```c
int main(void) {
    static int x = (sizeof(int) == 4);
    return x - 1;   /* expect 0 */
}
```

**Logical AND:**
```c
int main(void) {
    static int x = 1 && 1;
    static int y = 1 && 0;
    return x - 1 + y;   /* expect 0 */
}
```

**Logical OR:**
```c
int main(void) {
    static int x = 0 || 5;
    return x - 1;   /* expect 0: OR normalizes to 0 or 1 */
}
```

**Ternary:**
```c
int main(void) {
    static int x = 1 ? 42 : 0;
    static int y = 0 ? 0 : 7;
    return x - 42 + y - 7;   /* expect 0 */
}
```

### Invalid

**Relational with non-constant (parser):**
```c
int f(int n) {
    enum { X = n > 0 };   /* INVALID: n is not an integer constant */
    return X;
}
int main(void) { return f(1); }
```
Expected error contains: `is not an integer constant expression`

**Ternary with non-constant condition (static init):**
```c
int g(int n) {
    static int x = n ? 1 : 0;   /* INVALID: n is not a constant */
    return x;
}
int main(void) { return g(1); }
```
Expected error contains: `must be a constant expression`

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — update "Through stage …" line; note that constant
  expressions now include the full C99 operator set (relational, equality,
  logical, ternary) in all constant-expression contexts; refresh test totals.
- **`docs/grammar.md`** — update the `<constant-integer-expression>` grammar
  to add the six new operator levels.
- Run the **`update-supplemental-docs`** skill.
- **`docs/self-compilation-report.md`** — record the stage-104 self-host run.
