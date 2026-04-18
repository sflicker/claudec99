# Stage 8.2 Transcript: Nested Scopes and Variable Shadowing

## Summary

Implemented nested block scopes and variable shadowing as defined in
`docs/stages/ClaudeC99-spec-stage-08-02-nested-scopes.md`.

The stage adds these semantics:

- Each `{ ... }` introduces a new scope. Scopes may be nested arbitrarily.
- A variable declared in a scope is visible in that scope and all nested
  (child) scopes.
- A variable in an inner scope may **shadow** an outer one with the same
  name.
- Name resolution walks from the innermost scope outward; the nearest
  declaration wins.
- Redeclaring a variable **in the same scope** remains an error.

All changes were made in `src/compiler.c`. The spec explicitly calls out
that there are **no grammar changes** and **no tokenizer changes**, so
this stage is purely a semantic / code-generation change.

## Changes Made

### What Did NOT Change

- **Tokenizer**: unchanged — no new tokens.
- **Grammar / Parser**: unchanged — `{ ... }` blocks and `int x = ...;`
  declarations already parse the way the spec requires.
- **AST**: unchanged — `AST_BLOCK` and `AST_DECLARATION` are reused.
- **Pretty-printer**: unchanged.

### Step 1: Scope tracking in `CodeGen`

Added a `scope_start` field to the `CodeGen` struct recording the index
into `locals[]` at which the current scope begins. Everything before
`scope_start` belongs to an enclosing scope:

```c
typedef struct {
    FILE *output;
    int label_count;
    LocalVar locals[MAX_LOCALS];
    int local_count;
    int stack_offset;
    int scope_start; /* index into locals[] where current scope begins */
} CodeGen;
```

`codegen_init` initializes it to 0, and `codegen_function` resets it to
0 at the start of each function, alongside the existing per-function
resets of `local_count` and `stack_offset`.

### Step 2: Innermost-wins name resolution

`codegen_find_var` used to scan `locals[]` forward and return the first
hit. With shadowing, the innermost (most recently declared) name must
win. Changed the scan direction to walk backward:

```c
static int codegen_find_var(CodeGen *cg, const char *name) {
    /* Walk backward so the innermost (most recently declared) shadow wins. */
    for (int i = cg->local_count - 1; i >= 0; i--) {
        if (strcmp(cg->locals[i].name, name) == 0)
            return cg->locals[i].offset;
    }
    return 0; /* not found (valid offsets start at 4) */
}
```

### Step 3: Scope-local duplicate check

The old duplicate-declaration guard used `codegen_find_var`, which
scanned every currently-visible local — that would have rejected legal
shadowing. Replaced it with a targeted scan limited to names added in
the current scope:

```c
if (node->type == AST_DECLARATION) {
    /* Duplicate check limited to the current scope only — shadowing is allowed. */
    for (int i = cg->scope_start; i < cg->local_count; i++) {
        if (strcmp(cg->locals[i].name, node->value) == 0) {
            fprintf(stderr, "error: duplicate declaration of variable '%s'\n", node->value);
            exit(1);
        }
    }
    ...
}
```

### Step 4: Remove the `allows_decl` restriction

Prior stages forbade declarations inside nested scopes via an
`allows_decl` flag threaded through `codegen_statement`. With nested
scopes legal, that restriction is obsolete. Dropped the parameter
entirely from the function signature and from every recursive call
(`if`, `while`, `for`, block), and removed the "declaration not
allowed in nested scope" error path.

### Step 5: Push/pop scope on `AST_BLOCK`

Block entry saves the scope boundary and the local count; block exit
pops any variables declared inside — they stop being visible to
surrounding code, matching the spec's visibility rule:

```c
} else if (node->type == AST_BLOCK) {
    int saved_scope_start = cg->scope_start;
    int saved_local_count = cg->local_count;
    cg->scope_start = cg->local_count;
    for (int i = 0; i < node->child_count; i++) {
        codegen_statement(cg, node->children[i], is_main);
    }
    /* Pop variables declared in this scope — they are no longer visible. */
    cg->local_count = saved_local_count;
    cg->scope_start = saved_scope_start;
}
```

Stack offsets themselves are not reused between sibling scopes — each
declaration simply gets its own slot. That's why Step 6 is needed.

### Step 6: Recursive `count_declarations`

`count_declarations` previously only counted top-level declarations in
the function body, which was sufficient when declarations in nested
scopes were illegal. Now that they're legal, the function must count
declarations anywhere in the body so the prologue allocates enough
stack slots:

```c
static int count_declarations(ASTNode *node) {
    if (!node) return 0;
    int count = (node->type == AST_DECLARATION) ? 1 : 0;
    for (int i = 0; i < node->child_count; i++) {
        count += count_declarations(node->children[i]);
    }
    return count;
}
```

### Step 7: Tests

Added three tests, drawn directly from the spec examples:

**Valid:**

| Test File                                    | What it checks                                               | Expected |
|----------------------------------------------|--------------------------------------------------------------|----------|
| `test_shadow_inner_scope__2.c`               | Inner `int a = 2;` shadows outer `int a = 1;`; returns inner | 2        |
| `test_outer_var_in_inner_scope__1.c`         | Outer `a` is readable in an inner block; value propagates    | 1        |

**Invalid:**

| Test File                                  | What it checks                                       | Expected error fragment |
|--------------------------------------------|------------------------------------------------------|-------------------------|
| `test_invalid_3__duplicate_declaration.c`  | `int x; int x;` inside the *same* nested block       | `duplicate declaration` |

The existing `test_invalid_2__duplicate_declaration.c` (top-level
redeclaration) continues to cover the same-scope rule at function
scope; the new invalid test covers the same-scope rule inside a
nested block.

## Test Results

```
-- valid tests --
PASS  test_shadow_inner_scope__2        (exit code: 2)
PASS  test_outer_var_in_inner_scope__1  (exit code: 1)
Results: 74 passed, 0 failed, 74 total

-- invalid tests --
PASS  test_invalid_1__undeclared_variable    (error contains: 'undeclared variable')
PASS  test_invalid_2__duplicate_declaration  (error contains: 'duplicate declaration')
PASS  test_invalid_3__duplicate_declaration  (error contains: 'duplicate declaration')
Results: 3 passed, 0 failed, 3 total
```

No regressions in either suite.

## Spec Notes

The spec was clear and internally consistent; no ambiguities came up
during implementation. One incidental consequence worth recording: the
spec only defines scope behavior for `{ ... }` blocks, so a declaration
appearing as a single-statement body (e.g. `if (c) int y = 5;`) still
binds into the enclosing scope. That matches C's rule and required no
special handling.
