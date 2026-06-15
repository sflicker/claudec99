# ClaudeC99 Stage 128 — Floating-Point Constant Expressions at File Scope

## Goal

Allow arithmetic constant expressions in file-scope `float`/`double` initializers.

---

## Currently Failing

```c
double TWO_PI = 3.14159 * 2.0;    /* error: requires a constant initializer */
float  HALF   = 1.0f / 2.0f;      /* error */
double SUM    = 1.5 + 0.25;       /* error */
double NEG_PI = -3.14159265;      /* works — unary minus on float literal */
```

Only a single FP literal, single integer literal, or negated integer literal
is accepted. Arithmetic expressions are rejected.

---

## Root Cause

In `src/parser.c`, the float/double global initializer branch calls
`parse_assignment_expression` and then checks whether the result is
`AST_FLOAT_LITERAL` or `AST_INT_LITERAL`. It has no evaluator for
binary expressions over FP operands.

---

## Fix: `eval_fp_const_expr`

Add a new compile-time FP evaluator alongside `eval_const_expr`. Unlike the
integer evaluator (which returns `long`), this one returns `double`.

### Grammar

```
fp_const_expr    := fp_add_expr
fp_add_expr      := fp_mult_expr ( ('+' | '-') fp_mult_expr )*
fp_mult_expr     := fp_unary_expr ( ('*' | '/') fp_unary_expr )*
fp_unary_expr    := ('-' | '+') fp_unary_expr | fp_primary
fp_primary       := FLOAT_LITERAL | DOUBLE_LITERAL | INT_LITERAL | '(' fp_const_expr ')'
```

### Implementation (`src/parser.c`)

Add forward declaration and three static functions before `eval_const_expr`:

```c
static double eval_fp_const_expr(Parser *parser, const char *context);

static double eval_fp_primary(Parser *parser, const char *context) {
    if (parser->current.type == TOKEN_FLOAT_LITERAL ||
        parser->current.type == TOKEN_DOUBLE_LITERAL) {
        double v = strtod(parser->current.value, NULL);
        parser->current = lexer_next_token(parser->lexer);
        return v;
    }
    if (parser->current.type == TOKEN_INT_LITERAL) {
        double v = (double)parser->current.long_value;
        parser->current = lexer_next_token(parser->lexer);
        return v;
    }
    if (parser->current.type == TOKEN_LPAREN) {
        parser->current = lexer_next_token(parser->lexer);
        double v = eval_fp_const_expr(parser, context);
        parser_expect(parser, TOKEN_RPAREN);
        return v;
    }
    PARSER_ERROR(parser,
        "error: %s requires a floating-point constant expression\n", context);
}

static double eval_fp_unary(Parser *parser, const char *context) {
    if (parser->current.type == TOKEN_MINUS) {
        parser->current = lexer_next_token(parser->lexer);
        return -eval_fp_unary(parser, context);
    }
    if (parser->current.type == TOKEN_PLUS) {
        parser->current = lexer_next_token(parser->lexer);
        return +eval_fp_unary(parser, context);
    }
    return eval_fp_primary(parser, context);
}

static double eval_fp_mult(Parser *parser, const char *context) {
    double v = eval_fp_unary(parser, context);
    while (parser->current.type == TOKEN_STAR ||
           parser->current.type == TOKEN_SLASH) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        double r = eval_fp_unary(parser, context);
        v = (op == TOKEN_STAR) ? v * r : v / r;
    }
    return v;
}

static double eval_fp_const_expr(Parser *parser, const char *context) {
    double v = eval_fp_mult(parser, context);
    while (parser->current.type == TOKEN_PLUS ||
           parser->current.type == TOKEN_MINUS) {
        int op = parser->current.type;
        parser->current = lexer_next_token(parser->lexer);
        double r = eval_fp_mult(parser, context);
        v = (op == TOKEN_PLUS) ? v + r : v - r;
    }
    return v;
}
```

### Call site (`src/parser.c` — float/double global initializer branch)

Replace the current `parse_assignment_expression` + type-check path with a
call to `eval_fp_const_expr`, convert the result to a string, and create an
`AST_FLOAT_LITERAL` node:

```c
} else if (decl->decl_type == TYPE_FLOAT ||
           decl->decl_type == TYPE_DOUBLE) {
    /* Stage 128: evaluate as an FP constant expression. */
    double fv = eval_fp_const_expr(parser, "float/double global initializer");
    char fp_buf[64];
    if (decl->decl_type == TYPE_FLOAT) {
        float f32 = (float)fv;
        snprintf(fp_buf, sizeof(fp_buf), "%.9g", (double)f32);
    } else {
        snprintf(fp_buf, sizeof(fp_buf), "%.17g", fv);
    }
    /* Ensure NASM sees a decimal point. */
    if (!strchr(fp_buf, '.') && !strchr(fp_buf, 'e') && !strchr(fp_buf, 'E')) {
        strncat(fp_buf, ".0", sizeof(fp_buf) - strlen(fp_buf) - 1);
    }
    init = parser_node(parser, AST_FLOAT_LITERAL,
                       lexer_store_bytes(parser->lexer, fp_buf, strlen(fp_buf)));
```

This removes the old `parse_assignment_expression` + unary-minus folding code
and replaces it with the evaluator, which handles all cases: single literals,
negated literals, binary expressions.

No codegen changes needed — the AST_FLOAT_LITERAL path in `codegen_add_global`
already handles the string correctly.

---

## Tests

**`test/valid/floating_point/test_double_global_arith__0.c`**:
```c
double PI = 3.14159265;
double TWO_PI = 3.14159265 * 2.0;
int main(void) { return (TWO_PI > 6.0 && TWO_PI < 7.0) ? 0 : 1; }
```

**`test/valid/floating_point/test_float_global_arith__0.c`**:
```c
float HALF = 1.0f / 2.0f;
int main(void) { return (HALF > 0.4f && HALF < 0.6f) ? 0 : 1; }
```

**`test/valid/floating_point/test_double_global_neg_lit__0.c`**:
```c
double NEG = -3.14;
int main(void) { return (NEG < 0.0) ? 0 : 1; }
```

**`test/valid/floating_point/test_double_global_expr_parens__0.c`**:
```c
double X = (1.0 + 2.0) * 3.0;
int main(void) { return (X > 8.9 && X < 9.1) ? 0 : 1; }
```

---

## Version

Bump `VERSION_STAGE` in `src/version.c` to `"01280000"`.

---

## Out of Scope

- FP constant expressions in non-global contexts (e.g., static local initializers,
  array dimensions) — deferred.
- Modulo (`%`) on FP — not defined in C99.
- `fmod`, `sqrt`, standard-library FP functions in constant expressions — not supported.
