# Stage 5 Implementation Transcript

## Task
Extend the ClaudeC99 compiler to support integer variables with declarations, assignments, and scoping, as defined in `ClaudeC99-spec-stage-05-integer-variables.md`.

---

## Step 0: Spec Summary and Change Analysis

### New Tokens
- `=` (assignment) — single `=` character, distinct from `==`

### Grammar Changes
1. **`<statement>`** — expanded with two new alternatives: `<declaration>` and `<assignment_statement>`
2. **`<declaration>`** — new: `"int" <identifier> [ "=" <expression> ] ";"`
3. **`<assignment_statement>`** — new: `<identifier> "=" <expression> ";"`
4. **`<primary_expression>`** — expanded: now includes `<identifier>` (variable reference)

### AST Changes
- `AST_DECLARATION` — variable name in value, optional initializer child
- `AST_ASSIGNMENT` — variable name in value, one child (RHS expression)
- `AST_VAR_REF` — variable name in value, leaf node in expressions

### Code Generation Changes
- Per-function symbol table mapping variable names to stack offsets
- Stack frame prologue: `push rbp; mov rbp, rsp; sub rsp, N`
- Declaration allocates stack space, optionally stores initial value
- Assignment stores evaluated RHS into variable's stack slot
- Variable reference loads value from stack slot into `eax`

### Semantics / Error Detection
- Variables must be declared before use (compile-time error: "undeclared variable")
- Duplicate declarations in same scope are an error ("duplicate declaration")
- Variables only allowed in function's outermost scope (not in nested blocks or if branches)
- Assignment is a statement only, not an expression
- Only `int` variables supported

---

## Step 1: Tokenizer Updates

### What changed and why
The tokenizer already handled `=` as the start of `==` by peeking at the next character. The change adds a fallthrough: if `=` is not followed by `=`, emit `TOKEN_ASSIGN` instead of falling through to `TOKEN_UNKNOWN`.

### Changes made

**1. Added `TOKEN_ASSIGN` to the `TokenType` enum** (between `TOKEN_BANG` and `TOKEN_EQ`):
```c
TOKEN_BANG,
TOKEN_ASSIGN,   // NEW
TOKEN_EQ,
TOKEN_NE,
```

**2. Added lexer rule for standalone `=`** in `lexer_next_token()`, inserted between the `==` check and the `!=` check:
```c
if (c == '=' && n == '=') { token.type = TOKEN_EQ; strcpy(token.value, "=="); lexer->pos += 2; return token; }
if (c == '=') { token.type = TOKEN_ASSIGN; token.value[0] = c; lexer->pos++; return token; }  // NEW
if (c == '!' && n == '=') { token.type = TOKEN_NE; strcpy(token.value, "!="); lexer->pos += 2; return token; }
```

The ordering matters: `==` is checked first (two-char match wins), then standalone `=` is emitted as `TOKEN_ASSIGN`.

### Verification
- Compiler builds successfully
- Existing stage 4 tests (return, add, equality, if/else) still pass with no regressions

---

## Step 2: AST + Parser Updates

### What changed and why
AST and parser changes were done together since the parser creates AST nodes and depends on the new types.

### AST changes

Three new node types added to the `ASTNodeType` enum:
```c
AST_EXPRESSION_STMT,
AST_DECLARATION,    // NEW
AST_ASSIGNMENT,     // NEW
AST_VAR_REF         // NEW
```

Node semantics:
- `AST_DECLARATION`: value = variable name, 0 children (uninitialized) or 1 child (initializer expression)
- `AST_ASSIGNMENT`: value = variable name, 1 child (RHS expression)
- `AST_VAR_REF`: value = variable name, 0 children (leaf node)

### Parser changes

**1. `parse_primary()`** — added `TOKEN_IDENTIFIER` case to create variable reference nodes:
```c
if (parser->current.type == TOKEN_IDENTIFIER) {
    Token token = parser_expect(parser, TOKEN_IDENTIFIER);
    return ast_new(AST_VAR_REF, token.value);
}
```
This allows variables to appear anywhere in expressions (e.g., `a + b`, `a * (a + 1)`). Also updated the error message from "expected integer literal or '('" to "expected expression".

**2. `parse_statement()`** — added two new cases at the top of the dispatch:

**Declaration** (when current token is `TOKEN_INT`):
```c
if (parser->current.type == TOKEN_INT) {
    parser->current = lexer_next_token(parser->lexer);
    Token name = parser_expect(parser, TOKEN_IDENTIFIER);
    ASTNode *decl = ast_new(AST_DECLARATION, name.value);
    if (parser->current.type == TOKEN_ASSIGN) {
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *init = parse_expression(parser);
        ast_add_child(decl, init);
    }
    parser_expect(parser, TOKEN_SEMICOLON);
    return decl;
}
```
Inside a block, `int` always means a variable declaration (not a function), so this doesn't conflict with `parse_function_decl()`.

**Assignment** (when current token is `TOKEN_IDENTIFIER`):
```c
if (parser->current.type == TOKEN_IDENTIFIER) {
    int saved_pos = parser->lexer->pos;
    Token saved_token = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    if (parser->current.type == TOKEN_ASSIGN) {
        /* It's an assignment */
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *expr = parse_expression(parser);
        parser_expect(parser, TOKEN_SEMICOLON);
        ASTNode *assign = ast_new(AST_ASSIGNMENT, saved_token.value);
        ast_add_child(assign, expr);
        return assign;
    }
    /* Not assignment — restore lexer state and fall through to expression_stmt */
    parser->lexer->pos = saved_pos;
    parser->current = saved_token;
}
```
Uses save/restore on the lexer position to peek one token ahead. If `=` follows the identifier, it's an assignment. Otherwise, the state is restored and the code falls through to expression statement parsing (where the identifier becomes a `AST_VAR_REF` via `parse_primary`).

### Verification
- Compiler builds successfully
- Parser handles declarations and assignments without crashing (codegen not yet updated, so output is incomplete but no errors)
- Existing stage 4 tests still pass

---

## Step 3: Code Generation Updates

### What changed and why
This was the largest change. The code generator needed a symbol table, stack frame management, and handlers for all three new AST node types, plus semantic error checking.

### New data structures

**`LocalVar` struct** — maps a variable name to its stack offset:
```c
#define MAX_LOCALS 64

typedef struct {
    char name[256];
    int offset;
} LocalVar;
```

**`CodeGen` struct** — extended with per-function symbol table:
```c
typedef struct {
    FILE *output;
    int label_count;
    LocalVar locals[MAX_LOCALS];
    int local_count;
    int stack_offset;
} CodeGen;
```

### New helper functions

**`codegen_find_var()`** — looks up a variable by name, returns its stack offset (0 = not found, since valid offsets start at 4):
```c
static int codegen_find_var(CodeGen *cg, const char *name) {
    for (int i = 0; i < cg->local_count; i++) {
        if (strcmp(cg->locals[i].name, name) == 0)
            return cg->locals[i].offset;
    }
    return 0;
}
```

**`codegen_add_var()`** — registers a new variable, assigns the next 4-byte stack slot:
```c
static int codegen_add_var(CodeGen *cg, const char *name) {
    cg->stack_offset += 4;
    strncpy(cg->locals[cg->local_count].name, name, 255);
    cg->locals[cg->local_count].name[255] = '\0';
    cg->locals[cg->local_count].offset = cg->stack_offset;
    cg->local_count++;
    return cg->stack_offset;
}
```

**`count_declarations()`** — pre-scans a block's direct children to count declarations for upfront stack sizing:
```c
static int count_declarations(ASTNode *block) {
    int count = 0;
    for (int i = 0; i < block->child_count; i++) {
        if (block->children[i]->type == AST_DECLARATION)
            count++;
    }
    return count;
}
```

### Updated: `codegen_expression()`

Added `AST_VAR_REF` handler — loads the variable's value from its stack slot into `eax`. Errors on undeclared variable:
```c
if (node->type == AST_VAR_REF) {
    int offset = codegen_find_var(cg, node->value);
    if (offset == 0) {
        fprintf(stderr, "error: undeclared variable '%s'\n", node->value);
        exit(1);
    }
    fprintf(cg->output, "    mov eax, [rbp - %d]\n", offset);
    return;
}
```

### Updated: `codegen_statement()`

**Signature change:** added `int allows_decl` parameter to enforce scope restrictions.

**New `AST_DECLARATION` handler:**
- Checks `allows_decl` flag (error if declaration in nested scope)
- Checks for duplicate declaration via `codegen_find_var()`
- Registers variable via `codegen_add_var()`
- If initializer present, evaluates expression and stores to `[rbp - offset]`

**New `AST_ASSIGNMENT` handler:**
- Looks up variable (error if undeclared)
- Evaluates RHS expression
- Stores result to `[rbp - offset]`

**Scope enforcement:**
- If-statement branches pass `allows_decl=0` to `codegen_statement()`
- Nested `AST_BLOCK` nodes pass `allows_decl=0` to their children

### Updated: `codegen_function()`

Rewritten to handle stack frames and scope:

1. **Reset** per-function symbol table (`local_count=0`, `stack_offset=0`)
2. **Pre-scan** the function body block for declaration count
3. **Compute** stack size (declaration count * 4, rounded up to 16-byte alignment)
4. **Emit prologue** (only when variables exist): `push rbp; mov rbp, rsp; sub rsp, N`
5. **Iterate** body block children directly with `allows_decl=1` (not through generic `AST_BLOCK` handler)

Key design decision: the function body block is iterated directly in `codegen_function()` rather than delegated to `codegen_statement()`. This means any `AST_BLOCK` encountered inside `codegen_statement()` is by definition a nested block and correctly passes `allows_decl=0` to its children.

### Example generated assembly

For `int x = 3; x = x + 4; return x;` (`test_assignment_2__7.c`):
```nasm
section .text
global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov eax, 3
    mov [rbp - 4], eax
    mov eax, [rbp - 4]
    push rax
    mov eax, 4
    pop rcx
    add eax, ecx
    mov [rbp - 4], eax
    mov eax, [rbp - 4]
    mov edi, eax
    mov eax, 60
    syscall
```

### Verification
- All 39 valid tests pass (including 11 new variable tests and all 28 pre-existing stage 4 tests)
- Both invalid tests correctly rejected with appropriate error messages

### Commit
`e019c0c` — "stage 5 implementation: integer variables with declarations, assignments, and scoping"

---

## Step 4: Test Runner Fixes

### What changed and why

When tests were moved from `test/` to `test/valid/` for stage 5, the `PROJECT_DIR` computation in both test runner scripts broke. `dirname "$SCRIPT_DIR"` resolved to `test/` instead of the project root, so the compiler binary couldn't be found.

Additionally, the spec introduced invalid test files in `test/invalid/` but there was no runner for them.

### Changes made

**1. Fixed `test/valid/run_tests.sh`** — changed `PROJECT_DIR` to navigate two levels up:
```bash
# Before:
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
# After:
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
```

**2. Fixed `test/valid/run_test.sh`** — same `PROJECT_DIR` fix:
```bash
# Before:
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
# After:
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
```

**3. Created `test/invalid/run_tests.sh`** — new test runner for invalid test cases:
- Loops through all `test_*.c` files in `test/invalid/`
- Extracts expected error fragment from filename (`test_<name>__<fragment>.c`)
- Converts underscores in the fragment to spaces for matching
- Compiles with the compiler and expects non-zero exit code
- Checks that stderr contains the expected error fragment (case-insensitive grep)
- Reports PASS/FAIL with summary

### Verification
- Valid test runner: 39 passed, 0 failed
- Invalid test runner: 2 passed, 0 failed
- Single test runner (`run_test.sh`) works correctly

### Commit
`454bba6` — "fix test runner paths and add invalid test runner for stage 5"

---

## Final Results

### All tests passing

**Valid tests (39/39):**
- 28 pre-existing stage 1-4 tests (return, arithmetic, equality, relational, unary, if/else, blocks)
- 11 new stage 5 variable tests (declarations, assignments, expressions with variables, if with variables)

**Invalid tests (2/2):**
- `test_invalid_1__undeclared_variable.c` — error: undeclared variable 'x'
- `test_invalid_2__duplicate_declaration.c` — error: duplicate declaration of variable 'x'

### Files modified
- `src/compiler.c` — tokenizer, parser, AST, and code generator updates
- `test/valid/run_tests.sh` — fixed PROJECT_DIR path
- `test/valid/run_test.sh` — fixed PROJECT_DIR path
- `test/invalid/run_tests.sh` — new invalid test runner

### Commits
1. `e019c0c` — stage 5 implementation
2. `454bba6` — test runner fixes
