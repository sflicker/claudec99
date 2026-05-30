# Stage 76 Kickoff: C99 Declarations in for Loop Initializers

## Summary of the Spec

Stage 76 adds support for C99 declarations in the initializer clause of `for` loops. This allows idiomatic C99 code like:
```C
for (int i = 0; i < n; i++)
```

Currently, the compiler only supports optional expressions followed by a semicolon in the for loop's init clause. With this stage, declarations are also permitted, and the declared variable's scope extends across the entire loop (initializer, condition, post-expression, and body).

**Scope**: Support these forms:
- Simple integer loop variable: `for (int i = 0; i < n; i++)`
- Post-increment: `for (int i = 0; i < n; ++i)`
- Pointer iteration: `for (int *p = head; p; p = p->next)`
- Multiple declarations: `for (int i = 0, j = 10; i < j; i++)`
- Const pointers: `for (const char *p = s; *p; p++)`

**Out of scope**: Restrict/volatile qualifiers on loop variables (not mentioned in the spec).

## Tokenizer Changes

None required. No new tokens needed.

## Parser Changes

**File**: `src/parser.c` — function `parse_for_statement`

The parser currently expects: `for ( [expression] ; [expression] ; [expression] ) statement`

With this stage, it becomes: `for ( for_init [expression] ; [expression] ) statement` where `for_init` is either a declaration or an optional expression followed by `;`.

**Implementation approach**:
1. Push a new scope (`parser->scope_depth++`) before parsing the init clause
2. At the init position, check if the next token is a type specifier (e.g., `int`, `char`, `const`, etc.)
3. If it is a type specifier, call `parse_statement()` to parse the declaration (which consumes the trailing `;`)
4. If it is not a type specifier, parse an optional expression followed by `;` (existing behavior)
5. Parse the optional condition expression followed by `;`
6. Parse the optional post-expression (no trailing `;`)
7. Parse the statement body
8. Call `parser_leave_scope(parser)` after the body to exit the for-scope

The scope must encompass the entire loop so that the declared variable is visible throughout.

## AST Changes

None required. Existing node types cover the new cases:
- `AST_FOR_STATEMENT` (children: [init, condition, update, body]) — handles both declaration and expression init
- `AST_DECLARATION` — declaration in the init position
- `AST_DECL_LIST` — multiple declarators from a single declaration (e.g., `int i = 0, j = 10`)
- `AST_EXPRESSION_STATEMENT` — expression-based init

## Code Generation Changes

**File**: `src/codegen.c` — function handling `AST_FOR_STATEMENT`

The codegen must allocate a new local scope for the loop and manage stack frame properly:

1. Save current `scope_start` and `local_count` (analogous to block statement handling)
2. Set `scope_start = local_count` to begin a new scope
3. Dispatch `children[0]` (init node):
   - If it is `AST_DECLARATION` or `AST_DECL_LIST`, call `codegen_statement()`
   - If it is an expression-based init, call `codegen_expression()`
4. Generate the loop structure:
   - `loop_start:` label
   - Emit condition test: if condition is false, jump to `loop_end:`
   - Emit body statement
   - `loop_continue:` label
   - Emit post-expression (update)
   - Jump to `loop_start:`
   - `loop_end:` label
5. Restore `local_count` and `scope_start` after the loop ends

**Version Update**: `src/version.c` — increment from `"00750600"` to `"00760000"`

## Test Plan

### Valid Tests (7 new)

#### Test 1: test_for_decl_basic
Basic loop with integer declaration.
```C
int main(void) {
    int total = 0;
    
    for (int i = 0; i < 6; i++) {
        total = total + i;
    }
    
    return total;    // expect 15
}
```

#### Test 2: test_for_decl_body_sees_var
Loop variable is visible in loop body.
```C
int main(void) {
    for (int i = 0; i < 10; i++) {
        if (i == 4) {
            return i;
        }
    }
    
    return 99;   // expect 4
}
```

#### Test 3: test_for_decl_post_sees_var
Loop variable is visible in post-expression.
```C
int main(void) {
    int count = 0;
    
    for (int i = 0; i < 5; i++) {
        count = count + 1;
    }
    
    return count;    // expect 5
}
```

#### Test 4: test_for_decl_multiple
Multiple declarators in initializer (comma-separated).
```C
int main(void) {
    int total = 0;
    
    for (int i = 0, j = 10; i < 4; i++) {
        total = total + i + j;
    }
    
    return total;    // expect 46
}
```

#### Test 5: test_for_decl_pointer
Pointer declaration in initializer.
```C
int main(void) {
    int xs[3];
    int total = 0;
    
    xs[0] = 10;
    xs[1] = 20;
    xs[2] = 12;
    
    for (int *p = xs; p < xs + 3; p = p + 1) {
        total = total + *p;
    }
    
    return total;     // expect 42
}
```

#### Test 6: test_for_decl_const_pointer
Const pointer declaration in initializer.
```C
int main(void) {
    const char *s = "hello";
    
    for (const char *p = s; *p; p = p + 1) {
        if (*p == 'o') {
            return 42;
        }
    }
    
    return 0;
}
```

#### Test 7: test_for_decl_shadow
Loop variable shadows outer variable.
```C
int main(void) {
    int i = 40;
    
    for (int i = 0; i < 2; i = i + 1) {
    }
    
    return i + 2;   // expect 42
}
```

### Invalid Tests (2 new)

#### Test 1: test_for_decl_scope_leak
Loop variable is out of scope after loop.
```C
int main(void) {
    for (int i = 0; i < 3; i++) {
    }
    
    return i;    // INVALID: i is out of scope
}
```

**Expected**: Compilation error (undefined identifier `i`).

#### Test 2: test_for_decl_invalid_type
Declaration with disallowed type (void).
```C
int main(void) {
    for (void x; 0; ) {
    }
    
    return 0;
}
```

**Expected**: Compilation error (void cannot be used as variable type).

### Pretty Printer Test (1 new)

Add a `test_print_ast_for_decl` test to verify pretty-printer output for a for loop with declaration:
```C
for (int i = 0; i < 5; i++)
    printf("%d\n", i);
```

## Implementation Order

1. Update `src/parser.c` — modify `parse_for_statement` to handle declarations
2. Update `src/codegen.c` — enhance `AST_FOR_STATEMENT` handler for scope management
3. Update `src/version.c` from `"00750600"` to `"00760000"`
4. Create integration test files (7 valid + 2 invalid + 1 pretty-printer)
5. Update `docs/grammar.md` to document the new grammar rules
6. Update `README.md` if needed (feature list or status table)

## Ambiguities and Decisions

- **Scope boundaries**: The for-scope must wrap the entire loop (init through body) so that variables declared in the init clause are visible in the condition, post-expression, and body. Scope is exited immediately after the closing paren of the body.

- **Multiple declarators**: When parsing `int i = 0, j = 10`, this is a single declaration with multiple declarators. The parser's existing `parse_statement()` and declarator logic should handle this naturally via `AST_DECL_LIST`.

- **Expression vs. declaration ambiguity**: Distinguish at the point after the opening paren: if the next token is a type keyword (int, char, etc.), parse as declaration; otherwise, parse as optional expression. The type-specifier check must account for storage class and type qualifiers.

- **Spec issues identified**:
  1. Grammar: `<for_int>` should be `<for_init>` (typo in the spec)
  2. Multiple declarations test: uses `int i=0;j=10;` but correct form is `int i=0, j=10` (comma-separated)
  3. Pointer test: `xs[3] = 12;` is out of bounds; should be `xs[2] = 12;`. Also `toal` should be `total`.
  4. Const pointer test: contains syntax errors (missing colon in declaration, malformed if statement)
  5. Continue test: `int = 0;` is missing the variable name `i`; should be `int i = 0;`
  6. Invalid scope test: function declared as `int void(void)` instead of `int main(void)`
  7. Codegen pseudocode: uses `goto label_start` but the label is named `loop_start`

- **Continue behavior**: The `continue` statement should jump to `loop_continue:` label (before the post-expression), which is the existing behavior. No special handling needed for for-scope continue.

- **Break behavior**: The `break` statement should jump to `loop_end:` label (after the loop), which is the existing behavior. No special handling needed.

