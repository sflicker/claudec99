# ClaudeC99 Stage 129 — Function Declarations at Block Scope and Extern Incomplete Arrays

## Goal

Two small C99 features that currently produce parse errors:

1. **Function declarations at block scope** — `void foo(int x);` inside `{}`
2. **Incomplete array types in `extern` declarations** — `extern int arr[];`

---

## Feature 1: Function Declarations at Block Scope

### Currently Failing

```c
int foo(int x);
int main(void) {
    int foo(int x);   /* error: expected ';', got '(' */
    return foo(42) - 42;
}
int foo(int x) { return x; }
```

### Root Cause

In `src/parser.c`, `parse_statement` reaches the block-scope declaration handler.
After `parse_type_specifier` + `parse_declarator`, if `d.is_function == 1`, there
is no handler for it. The code falls through to try to parse an expression
declaration, encounters the `(` token, and errors.

`parse_declarator` sets `d.is_function = 1` and leaves the current token at `(`
(the parameter list open paren) without consuming it.

### Fix

In the block-scope declaration block (around line 3101 in `src/parser.c`), add a
`d.is_function` branch before the variable-declaration handling:

```c
if (d.is_function) {
    /* Block-scope function declaration — C99 §6.2.1p4.
     * Consume the parameter list, expect semicolon, register the function,
     * and return a no-op node. No codegen output is produced. */
    /* Consume '(' */
    parser_expect(parser, TOKEN_LPAREN);
    /* Parse and discard parameter list: balance parens, stop at ')' */
    int depth = 1;
    while (depth > 0 && parser->current.type != TOKEN_EOF) {
        if (parser->current.type == TOKEN_LPAREN) depth++;
        else if (parser->current.type == TOKEN_RPAREN) { depth--; if (!depth) break; }
        parser->current = lexer_next_token(parser->lexer);
    }
    parser_expect(parser, TOKEN_RPAREN);
    parser_expect(parser, TOKEN_SEMICOLON);
    /* Register in the function table if not already present (allows later calls). */
    if (!parser_find_function(parser, d.name)) {
        /* Register with 0 params and the declared return type. */
        parser_register_function(parser, d.name, -1, 0, base_kind, NULL, SC_NONE, 0);
    }
    return parser_node(parser, AST_TYPEDEF_DECL, "");
}
```

**Why `param_count = -1`?** The existing `parser_register_function` treats `-1`
as "variadic/unknown" and does not enforce arity checks at call sites. This is
conservative — the function is forward-declared but no parameter type checking
is done. Since the file-scope definition will be processed later, this is safe.

Actually, simpler: just skip registration entirely if the function is already
declared. If not declared, register with 0-param-sentinel that allows any call.

### Test

```c
/* test/valid/functions/test_block_scope_fn_decl__0.c */
static int add(int a, int b) { return a + b; }
int main(void) {
    int add(int a, int b);   /* block-scope re-declaration */
    return add(3, 4) - 7;
}
```

Expected exit: 0.

---

## Feature 2: Extern Incomplete Arrays

### Currently Failing

```c
extern int arr[];           /* error: array size required for file-scope declaration 'arr' */
int arr[5] = {1,2,3,4,5};
int main(void) { return arr[0] - 1; }
```

### Root Cause

In `src/parser.c`, the file-scope array declaration path (around line 3808):

```c
} else if (!has_size) {
    PARSER_ERROR(parser,
            "error: array size required for file-scope declaration '%s'\n",
            d.name);
}
```

This error fires for any array without a size, including `extern` ones.

### Fix

Check `sc == SC_EXTERN` before erroring. An `extern` array with no size is an
incomplete type declaration — it's a reference to an array defined elsewhere
whose complete type (size) is known at link time. The compiler emits `extern name`
in the assembly and uses it like a pointer.

```c
} else if (!has_size) {
    if (sc != SC_EXTERN) {
        PARSER_ERROR(parser,
                "error: array size required for file-scope declaration '%s'\n",
                d.name);
    }
    /* Stage 129: extern incomplete array — treat as pointer-sized extern ref.
     * Use length=0 as sentinel; codegen emits 'extern name'. */
    length = 0;
    has_size = 1;
}
```

For codegen: an `extern int arr[]` with `length = 0` should emit `extern arr`
in the `.text` section preamble (same as any other `is_extern` global). The
existing `codegen_emit_externs` path already handles `is_extern` globals correctly.

**What about element accesses?** When `arr[i]` is emitted, codegen uses
`codegen_find_global` to find `arr`, checks its kind (`TYPE_ARRAY`) and element
size. With `length = 0`, the array bounds check (if any) would need to be
suppressed. Since this compiler doesn't emit runtime bounds checks, accessing
`arr[i]` for an incomplete extern array works the same as a complete one.

### Test

```c
/* test/valid/arrays/test_extern_incomplete_array__0.c */
extern int data[];
int data[5] = {10, 20, 30, 40, 50};
int main(void) { return data[2] - 30; }
```

Expected exit: 0.

---

## Version

Bump `VERSION_STAGE` in `src/version.c` to `"01290000"`.

---

## Out of Scope

- Block-scope function *definitions* — not valid C99.
- Parameter type checking for block-scope function declarations — deferred.
- Incomplete struct/union types in extern declarations — separate issue.
