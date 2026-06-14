# ClaudeC99 Stage 124 — Octal Integer Literals, `__func__`, File-Scope Compound Literals

## Goal

Three independent C99 language gap fills in one stage:

| Task | Feature | Where |
|------|---------|--------|
| 1 | Octal integer literals (`0755`, `0644`) | `src/lexer.c` |
| 2 | `__func__` predefined identifier | `src/codegen.c` |
| 3 | File-scope compound literals (`int *p = (int[]){1,2,3}`) | `src/parser.c` + `src/codegen.c` |

None of these interact.

---

## Task 1 — Octal Integer Literals

### Root Cause

The lexer tokenizes integers with the flag `is_hex ? 16 : 10` (line 581 of `src/lexer.c`). There is no octal branch. A literal like `0755` is parsed as decimal 755 instead of octal 493.

Stage 90 added hexadecimal literals (`0x`/`0X`). Stage 88 added octal *escape sequences* (`\NNN` in characters and strings). Octal *integer literals* were never added.

### Fix (`src/lexer.c`)

The existing structure in the integer-literal branch (line 488 onward):

```c
if (c == '0' && (lexer->source[lexer->pos + 1] == 'x' ||
                 lexer->source[lexer->pos + 1] == 'X')) {
    is_hex = 1;
    /* consume 0x, read hex digits ... */
} else {
    /* read decimal digits ... */
}
```

Add an `is_octal` path between hex detection and the decimal fallthrough:

```c
} else if (c == '0' && isdigit((unsigned char)lexer->source[lexer->pos])) {
    is_octal = 1;
    /* Read octal digits 0-7. Stop (and error) on 8 or 9. */
    while (isdigit((unsigned char)lexer->source[lexer->pos]) && i < 62) {
        char dc = lexer->source[lexer->pos];
        if (dc == '8' || dc == '9') {
            fprintf(stderr, "error: invalid digit '%c' in octal constant\n", dc);
            exit(1);
        }
        num_buf[i++] = dc;
        lexer_advance(lexer);
    }
} else {
    /* decimal */
}
```

Change the `strtoul` call base:

```c
unsigned long parsed = strtoul(num_buf, &end,
                               is_hex ? 16 : (is_octal ? 8 : 10));
```

**Edge cases:**
- `0` alone — still enters the decimal branch (next char is not a digit), parses as 0. No change needed.
- `08` / `09` — error: "invalid digit '8' in octal constant".
- `0755UL` — suffix parsing (U, L, LL) is unchanged; it runs after the digit loop regardless of base.

**Grammar:**
```
<integer_literal> ::= <decimal_literal> | <hex_literal> | <octal_literal>
<octal_literal>   ::= "0" [0-7]* <integer_suffix>?
```

### Tests

- `test/valid/arithmetic/test_octal_basic__493.c` — `return 0755;`
- `test/valid/arithmetic/test_octal_permission__6.c` — `return (0644 >> 6) & 7;`
- `test/valid/arithmetic/test_octal_zero__0.c` — `return 00;` (trivial, regression guard)
- `test/invalid/expressions/test_invalid_octal_8__*.c` — `int x = 08;` → error
- `test/invalid/expressions/test_invalid_octal_9__*.c` — `int x = 079;` → error

---

## Task 2 — `__func__` Predefined Identifier

### Specification

C99 §6.4.2.2p1:

> The identifier `__func__` shall be implicitly declared by the translator as if, immediately following the opening brace of each function definition, the declaration
> ```c
> static const char __func__[] = "function-name";
> ```
> appeared.

`__func__` is used in `assert()` macros, logging helpers, and debugging.

### Implementation (`src/codegen.c`)

Inject a synthetic `LocalStaticVar` + `LocalVar` at the start of `codegen_function` body processing, immediately before the parameter-slot loop. At that point `cg->current_func` (or `node->value`) holds the function name.

**Step A — Generate a label** using the existing counter:
```c
char label_buf[256];
snprintf(label_buf, sizeof(label_buf), "Lstatic_%s___func__",
         cg->current_func);
/* Label must be unique per TU. Use label_count for safety. */
snprintf(label_buf, sizeof(label_buf), "Lstatic_%s___func__%d",
         cg->current_func, cg->label_count++);
const char *func_label = codegen_intern(cg, label_buf);
```

**Step B — Build a synthetic `AST_STRING_LITERAL` node** for the function name:
```c
ASTNode *str = ast_new_node(AST_STRING_LITERAL);
str->value       = node->value;          /* points to function name in lexer pool */
str->byte_length = strlen(node->value) + 1;
```

**Step C — Push a `LocalStaticVar`**:
```c
LocalStaticVar func_sv;
func_sv.label        = func_label;
func_sv.kind         = TYPE_ARRAY;
func_sv.full_type    = type_array(type_char(), (int)strlen(node->value) + 1);
func_sv.size         = (int)strlen(node->value) + 1;
func_sv.is_initialized = 1;
func_sv.init_value   = 0;
func_sv.is_unsigned  = 0;
func_sv.init_node    = str;
func_sv.is_label_init = 0;
func_sv.init_label   = NULL;
vec_push(&cg->local_statics, &func_sv);
```

This reuses the existing `TYPE_ARRAY + AST_STRING_LITERAL` path in `codegen_emit_local_statics`, which emits:
```asm
Lstatic_main___func__0: db "main", 0
```

**Step D — Push a `LocalVar`**:
```c
LocalVar func_lv;
func_lv.name         = "__func__";
func_lv.offset       = 0;    /* unused for statics */
func_lv.size         = 8;    /* pointer size (static array decays) */
func_lv.kind         = TYPE_ARRAY;
func_lv.full_type    = func_sv.full_type;
func_lv.is_const     = 1;
func_lv.is_unsigned  = 0;
func_lv.is_static    = 1;
func_lv.static_label = func_label;
vec_push(&cg->locals, &func_lv);
```

With `is_static = 1` and `kind = TYPE_ARRAY`, the existing `AST_VAR_REF` codegen path emits `lea rax, [rel label]` — the address of the string — when `__func__` is used in an expression. Array-to-pointer decay produces the correct `const char *` result.

### User Shadowing

If the user explicitly declares `__func__` (undefined behavior per C99 §6.4.2.2p2), the later user declaration in `cg->locals` shadows the implicit one because `codegen_find_var` searches from the end of the vector. No special handling required.

### Tests (go in `test/valid/functions/`)

Tests that check stdout need a `.expected` companion file.

**`test_func_main__0.c`** + **`test_func_main.expected`**:
```c
#include <stdio.h>
int main(void) {
    printf("%s\n", __func__);
    return 0;
}
```
`.expected`:
```
main
```

**`test_func_in_helper__0.c`** + **`test_func_in_helper.expected`**:
```c
#include <stdio.h>
#include <string.h>
static void helper(void) { printf("%s\n", __func__); }
int main(void) { helper(); return 0; }
```
`.expected`:
```
helper
```

**`test_func_as_strcmp__0.c`**:
```c
#include <string.h>
static int check(void) { return strcmp(__func__, "check"); }
int main(void) { return check(); }
```
Expected exit: 0 (strcmp returns 0 when equal).

---

## Task 3 — File-Scope Compound Literals

### Root Cause

Stage 98 added block-scope compound literals (`(T){ ... }`) and explicitly rejected file-scope compound literals with an error at `src/codegen.c:5092`:
```c
compile_error("error: compound literals at file scope are not yet supported\n");
```

C99 §6.5.2.5p3: a compound literal outside a function body has **static storage duration** and can be used as a pointer initializer:
```c
int *p = (int[]){1, 2, 3};          /* array decays to pointer */
struct S *q = &(struct S){0, 0};    /* struct address taken */
int *r = &(int){42};                /* scalar address taken */
```

The parser additionally rejects `AST_COMPOUND_LITERAL` as a global pointer initializer (the node type is not in the accepted list from stage 123).

### Fix — Parser (`src/parser.c`)

In `parse_external_declaration`, the pointer global initializer validation:

**Before** (from stage 123):
```c
if (init->type != AST_INT_LITERAL && init->type != AST_CHAR_LITERAL &&
    init->type != AST_STRING_LITERAL && init->type != AST_VAR_REF &&
    init->type != AST_ADDR_OF) {
    PARSER_ERROR(...);
}
```

**After**:
```c
if (init->type != AST_INT_LITERAL && init->type != AST_CHAR_LITERAL &&
    init->type != AST_STRING_LITERAL && init->type != AST_VAR_REF &&
    init->type != AST_ADDR_OF &&
    init->type != AST_COMPOUND_LITERAL) {
    PARSER_ERROR(...);
}
```

### Fix — Codegen: `codegen_add_global` (`src/codegen.c`)

Add a case for `AST_COMPOUND_LITERAL` init nodes on pointer globals:
```c
} else if (gv->kind == TYPE_POINTER && init->type == AST_COMPOUND_LITERAL) {
    gv->init_node = init;
    gv->is_initialized = 1;
}
```

### Fix — Codegen: `codegen_emit_data` (`src/codegen.c`)

Add a `compound_literal_count` field to `CodeGen` (initialized to 0 in `codegen_init`), then add a case before the existing `is_label_init` fallthrough.

**Case A — Array compound literal decaying to pointer** (`(T[]){...}`):
```c
} else if (gv->kind == TYPE_POINTER && gv->init_node &&
           gv->init_node->type == AST_COMPOUND_LITERAL &&
           gv->init_node->full_type &&
           gv->init_node->full_type->kind == TYPE_ARRAY) {
    /* Emit the array data as an anonymous static object, then dq its label. */
    char anon_buf[64];
    snprintf(anon_buf, sizeof(anon_buf), "Lcompound_%d",
             cg->compound_literal_count++);
    const char *anon = codegen_intern(cg, anon_buf);
    ASTNode *lit = gv->init_node;
    Type *arr_type = lit->full_type;
    int arr_len = arr_type->length;
    Type *elem_type = arr_type->base;
    const char *dir = data_init_directive(elem_type->kind);
    fprintf(cg->output, "%s:\n", anon);
    /* Emit elements from the initializer list child. */
    ASTNode *list = (lit->child_count > 0) ? lit->children[0] : NULL;
    int j;
    for (j = 0; j < arr_len; j++) {
        if (list && j < list->child_count) {
            ASTNode *e = list->children[j];
            if (e->type == AST_INT_LITERAL) {
                long v = strtol(e->value, NULL, 0);
                fprintf(cg->output, "    %s %ld\n", dir, v);
            } else if (e->type == AST_CHAR_LITERAL) {
                long v = (long)(unsigned char)e->value[0];
                fprintf(cg->output, "    %s %ld\n", dir, v);
            } else {
                compile_error("error: unsupported element in file-scope compound literal\n");
            }
        } else {
            fprintf(cg->output, "    %s 0\n", dir);
        }
    }
    if (!gv->is_static_linkage)
        fprintf(cg->output, "global %s\n", gv->name);
    fprintf(cg->output, "%s: dq %s\n", gv->name, anon);
```

**Case B — `AST_ADDR_OF` wrapping `AST_COMPOUND_LITERAL`** (already stored via stage 123's `AST_ADDR_OF` branch in `codegen_add_global`): extend the existing `AST_ADDR_OF` handling in `codegen_emit_data` to handle a compound-literal child:

```c
} else if (child && child->type == AST_COMPOUND_LITERAL) {
    char anon_buf[64];
    snprintf(anon_buf, sizeof(anon_buf), "Lcompound_%d",
             cg->compound_literal_count++);
    const char *anon = codegen_intern(cg, anon_buf);
    ASTNode *lit = child;
    if (lit->full_type && lit->full_type->kind == TYPE_STRUCT) {
        fprintf(cg->output, "%s:\n", anon);
        ASTNode *list = (lit->child_count > 0) ? lit->children[0] : NULL;
        if (list) emit_global_struct(cg, lit->full_type, list);
    } else if (lit->full_type == NULL || lit->full_type->kind == TYPE_INT ||
               lit->full_type->kind == TYPE_LONG) {
        /* Scalar compound literal: (int){42} */
        ASTNode *val = (lit->child_count > 0) ? lit->children[0] : NULL;
        long v = (val && val->type == AST_INT_LITERAL)
                 ? strtol(val->value, NULL, 0) : 0;
        fprintf(cg->output, "%s: %s %ld\n",
                anon, data_init_directive(lit->decl_type), v);
    } else {
        compile_error("error: unsupported type in address-of compound literal\n");
    }
    fprintf(cg->output, "%s: dq %s\n", gv->name, anon);
}
```

### Guard in `codegen_expression`

The existing guard at line 5089–5092:
```c
if (node->type == AST_COMPOUND_LITERAL) {
    if (!cg->in_function) {
        compile_error("error: compound literals at file scope are not yet supported\n");
    }
```

This fires when `codegen_expression` encounters a compound literal outside a function. Since file-scope initializers bypass `codegen_expression` entirely (they store the node and emit in `codegen_emit_data`), this guard will only trigger if a compound literal somehow reaches `codegen_expression` from a non-initializer context — which is impossible in valid file-scope C. Remove the `!cg->in_function` guard (or leave it; it won't be reached for the supported cases).

### Tests (go in `test/valid/pointers/`)

- **`test_file_scope_array_literal__30.c`**:
  ```c
  int *p = (int[]){10, 20, 30};
  int main(void) { return p[0] + p[1] + p[2] - 30; }
  ```
  Expected exit: **30** (10+20+30 - 30 = 30)

- **`test_file_scope_struct_literal__5.c`**:
  ```c
  struct S { int x; int y; };
  struct S *q = &(struct S){3, 5};
  int main(void) { return q->y; }
  ```
  Expected exit: **5**

- **`test_file_scope_scalar_literal__42.c`**:
  ```c
  int *r = &(int){42};
  int main(void) { return *r; }
  ```
  Expected exit: **42**

---

## Version

Bump `VERSION_STAGE` in `src/version.c` to `"01240000"`.

---

## Implementation Order

1. Task 1 (octal lexer) — build, run octal tests.
2. Task 2 (`__func__`) — build, run `__func__` tests.
3. Task 3 (file-scope compound literals) — parser change, codegen change, run tests.
4. Full suite: `./test/run_all_tests.sh`.
5. Bump `src/version.c`.
6. Self-host: `./build.sh --mode=self-host`.
7. Commit.

---

## Out of Scope

- **Scalar compound literals as integer initializers**: `int x = (int){42};` at file scope. The value could be constant-folded, but this pattern is unusual and not worth extending `eval_const_expr` to handle compound literal nodes. The existing "non-constant initializer" error remains.
- **Nested compound literals in initializer lists**: `int **pp = &(int*[]){p, q};` — complex interaction with existing array slot logic.
- **Compound literals of FP type**: `double *p = &(double){3.14};` — the struct/array case above covers integer types. FP compound literals at file scope require the FP-globals codegen fixes from stage 125.
