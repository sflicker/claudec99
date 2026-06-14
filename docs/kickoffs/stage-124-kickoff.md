# Stage 124 Kickoff — Octal Integer Literals, `__func__`, File-Scope Compound Literals

## Overview

Stage 124 implements three independent C99 language features:

1. **Octal integer literals** (`0755`, `0644`) — lexer support for base-8 digit sequences
2. **`__func__` predefined identifier** — implicit string constant injected at function start
3. **File-scope compound literals** (`int *p = (int[]){1,2,3}`) — static-storage compound literals as global initializers

These tasks have no interdependencies and can be implemented in sequence.

## Spec Summary

**Task 1 — Octal Integer Literals**
- Detection: A `0` followed by digits 0-7 (not 8-9)
- Standalone `0` stays decimal; `08`/`09` error
- Suffix parsing (U, L, LL) unchanged
- Use `strtoul` with base 8 instead of base 10

**Task 2 — `__func__` Predefined Identifier**
- C99 §6.4.2.2p1: implicit `static const char __func__[] = "function-name"`
- Inject at start of function body processing
- Build synthetic `LocalStaticVar` + `LocalVar` pair
- Reuses existing local-statics emission path
- User shadowing works automatically via vec search order

**Task 3 — File-Scope Compound Literals**
- Remove parser rejection of `AST_COMPOUND_LITERAL` in pointer global initializers
- Add codegen cases for:
  - Array compound literal decaying to pointer: emit anonymous static array, then dq its label
  - Address-of wrapping compound literal: extend existing `AST_ADDR_OF` handling
- Introduce `compound_literal_count` field to `CodeGen` for label generation

---

## Required Changes

### 1. Tokenizer Changes (`src/lexer.c`)

**File:** `src/lexer.c`

In the integer-literal branch (around line 488), add an `is_octal` flag and detection:

- After hex detection (`is_hex`), add:
  ```c
  } else if (c == '0' && isdigit((unsigned char)lexer->source[lexer->pos])) {
      is_octal = 1;
      /* Read octal digits 0-7. Error on 8 or 9. */
  }
  ```

- During digit loop, reject 8 and 9 with error message

- Change `strtoul` base parameter:
  ```c
  is_hex ? 16 : (is_octal ? 8 : 10)
  ```

**Edge Cases:**
- `0` alone → decimal (next char is not a digit)
- `08` / `09` → error: "invalid digit '8' in octal constant"
- `0755UL` → suffix parsing unchanged

### 2. Parser Changes (`src/parser.c`)

**File:** `src/parser.c`

In `parse_external_declaration`, extend the pointer global initializer validation to accept `AST_COMPOUND_LITERAL`:

```c
if (init->type != AST_INT_LITERAL && init->type != AST_CHAR_LITERAL &&
    init->type != AST_STRING_LITERAL && init->type != AST_VAR_REF &&
    init->type != AST_ADDR_OF &&
    init->type != AST_COMPOUND_LITERAL) {  // ADD THIS LINE
    PARSER_ERROR(...);
}
```

### 3. AST Changes

**No changes required.** Stage 124 uses existing AST node types: `AST_COMPOUND_LITERAL`, `AST_ADDR_OF`, `AST_STRING_LITERAL`, `AST_INT_LITERAL`.

### 4. Code Generation Changes (`src/codegen.c`, `include/codegen.h`)

#### 4a. Header Change (`include/codegen.h`)

Add field to `CodeGen` struct:
```c
int compound_literal_count;  // Counter for anonymous compound literal labels
```

#### 4b. Initialization (`src/codegen.c`, `codegen_init`)

Initialize in `codegen_init`:
```c
cg->compound_literal_count = 0;
```

#### 4c. `__func__` Injection (`src/codegen.c`, `codegen_function`)

At the start of function body processing (before parameter-slot loop), inject synthetic locals:

**Step A — Generate label:**
```c
char label_buf[256];
snprintf(label_buf, sizeof(label_buf), "Lstatic_%s___func__%d",
         cg->current_func, cg->label_count++);
const char *func_label = codegen_intern(cg, label_buf);
```

**Step B — Build string literal node:**
```c
ASTNode *str = ast_new_node(AST_STRING_LITERAL);
str->value       = node->value;          // function name
str->byte_length = strlen(node->value) + 1;
```

**Step C — Push LocalStaticVar:**
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
vec_push(&cg->locals, &func_sv);
```

**Step D — Push LocalVar:**
```c
LocalVar func_lv;
func_lv.name         = "__func__";
func_lv.offset       = 0;    // unused for statics
func_lv.size         = 8;    // pointer size
func_lv.kind         = TYPE_ARRAY;
func_lv.full_type    = func_sv.full_type;
func_lv.is_const     = 1;
func_lv.is_unsigned  = 0;
func_lv.is_static    = 1;
func_lv.static_label = func_label;
vec_push(&cg->locals, &func_lv);
```

#### 4d. File-Scope Compound Literals (`src/codegen.c`)

**In `codegen_add_global`:**
Add case for pointer global with compound-literal init:
```c
} else if (gv->kind == TYPE_POINTER && init->type == AST_COMPOUND_LITERAL) {
    gv->init_node = init;
    gv->is_initialized = 1;
}
```

**In `codegen_emit_data`:**
Add two cases before the existing `is_label_init` fallthrough.

**Case A — Array compound literal:**
```c
} else if (gv->kind == TYPE_POINTER && gv->init_node &&
           gv->init_node->type == AST_COMPOUND_LITERAL &&
           gv->init_node->full_type &&
           gv->init_node->full_type->kind == TYPE_ARRAY) {
    // Emit anonymous array, then dq its label
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

**Case B — Address-of compound literal:**
Extend the existing `AST_ADDR_OF` handling to detect compound-literal child:
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
        // Scalar compound literal: (int){42}
        ASTNode *val = (lit->child_count > 0) ? lit->children[0] : NULL;
        long v = (val && val->type == AST_INT_LITERAL)
                 ? strtol(val->value, NULL, 0) : 0;
        fprintf(cg->output, "%s: %s %ld\n",
                anon, data_init_directive(lit->full_type->kind), v);
    } else {
        compile_error("error: unsupported type in address-of compound literal\n");
    }
    fprintf(cg->output, "%s: dq %s\n", gv->name, anon);
}
```

#### 4e. Version Bump (`src/version.c`)

Change `VERSION_STAGE` from current version to:
```c
#define VERSION_STAGE "01240000"
```

---

## Test Plan

### Task 1 Tests — Octal Integer Literals

**Valid tests** (exit code equals value):
- `test/valid/arithmetic/test_octal_basic__493.c` — `return 0755;` (octal 493 = decimal 493)
- `test/valid/arithmetic/test_octal_permission__6.c` — `return (0644 >> 6) & 7;` (expected 6)
- `test/valid/arithmetic/test_octal_zero__0.c` — `return 00;` (regression guard)

**Invalid tests** (expect compile error):
- `test/invalid/expressions/test_invalid_octal_8__*.c` — `int x = 08;` → error
- `test/invalid/expressions/test_invalid_octal_9__*.c` — `int x = 079;` → error

### Task 2 Tests — `__func__` Predefined Identifier

**Valid tests** (in `test/valid/functions/`):
- `test_func_main__0.c` + `.expected` — `printf("%s\n", __func__);` → outputs "main"
- `test_func_in_helper__0.c` + `.expected` — helper function prints "__func__" → outputs "helper"
- `test_func_as_strcmp__0.c` — `strcmp(__func__, "check")` in function `check` → exit 0

### Task 3 Tests — File-Scope Compound Literals

**Valid tests** (in `test/valid/pointers/`):
- `test_file_scope_array_literal__30.c` — `int *p = (int[]){10, 20, 30};` then `return p[0] + p[1] + p[2] - 30;` → 30
- `test_file_scope_struct_literal__5.c` — `struct S *q = &(struct S){3, 5};` then `return q->y;` → 5
- `test_file_scope_scalar_literal__42.c` — `int *r = &(int){42};` then `return *r;` → 42

---

## Implementation Order

1. Task 1 (octal lexer) — build, run octal tests
2. Task 2 (`__func__`) — build, run `__func__` tests
3. Task 3 (file-scope compound literals) — parser + codegen, run tests
4. Full suite: `./test/run_all_tests.sh`
5. Bump `src/version.c`
6. Self-host: `./build.sh --mode=self-host`
7. Commit with Co-Authored-By trailer

---

## Notes & Decisions

- **No AST changes needed**: existing node types suffice
- **`is_octal` initialization**: remember to initialize the flag to 0 at the start of the integer-literal branch
- **Label generation**: use existing `cg->label_count++` for `__func__` uniqueness; introduce new `compound_literal_count` for file-scope compound literals
- **User shadowing of `__func__`**: already handled by vector search order in `codegen_find_var`
- **Scalar compound literal initializers** (`int x = (int){42};`) out of scope — would require constant-folding logic
