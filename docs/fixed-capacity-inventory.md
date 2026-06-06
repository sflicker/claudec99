# Fixed-Capacity Table Inventory

Produced in Stage 95-01. Future stages should mark completed items with ✓ in the Status column.

Columns:
- **Name** — constant or table name
- **Max** — current default capacity
- **Module** — file(s) where defined and used
- **On Overflow** — what happens when the limit is exceeded
- **Ext Ptr Refs** — whether live pointers into the array escape the immediate call frame
- **Safe Realloc** — whether entries can move after a realloc (YES = no escaping pointers; NO = escaping aliases; N/A = not heap-allocated)
- **Priority** — relative urgency for the dynamic-capacity work plan
- **Status** — PENDING until a future stage replaces with dynamic storage

---

## String Name Buffers

| Name | Max | Module | On Overflow | Ext Ptr Refs | Safe Realloc | Priority | Status |
|------|-----|--------|-------------|--------------|--------------|----------|--------|
| `MAX_NAME_LEN` | 256 bytes | `include/constants.h`; used in `char name[]`, `char tag[]`, `char label[]`, `char value[]`, `char static_label[]`, `char init_label[]` fields in `ASTNode`, `LocalVar`, `GlobalVar`, `LocalStaticVar`, `FuncSig`, `GlobalObjSig`, `TypedefEntry`, `EnumConst`, `EnumTag`, `StructTag`, `UnionTag`, `StructField`, and `ParsedDeclarator` | `strncpy` silently truncates to 255 bytes; the identifier stored in the struct is silently wrong | No | N/A (embedded `char[]`) | LOW | PENDING |

---

## AST

| Name | Max | Module | On Overflow | Ext Ptr Refs | Safe Realloc | Priority | Status |
|------|-----|--------|-------------|--------------|--------------|----------|--------|
| `AST_MAX_CHILDREN` | 64 (initial capacity) | `include/constants.h`; `src/ast.c` | **Already dynamic** — `ast_add_child` doubles the allocation when full; this constant is only the initial capacity. Fixed in stage 92. | N/A | N/A | ✓ DONE (stage 92) | — |

---

## Type System

| Name | Max | Module | On Overflow | Ext Ptr Refs | Safe Realloc | Priority | Status |
|------|-----|--------|-------------|--------------|--------------|----------|--------|
| `FUNC_TYPE_MAX_PARAMS` | 16 | `include/constants.h`; `include/type.h` (`Type.params[]`); `src/parser.c` (checked at line 1198) | `PARSER_ERROR` — "too many function parameters" | No — `Type.params[]` is an embedded array inside a heap-allocated `Type`; callers receive a `Type *`, not a pointer into the `params` array | NO (embedded in struct; realloc of the array would change `Type` layout; params would need to be a separately-heap-allocated array) | LOW | PENDING |

---

## Parser

| Name | Max | Module | On Overflow | Ext Ptr Refs | Safe Realloc | Priority | Status |
|------|-----|--------|-------------|--------------|--------------|----------|--------|
| `PARSER_MAX_FUNCTIONS` | 256 | `include/constants.h`; `include/parser.h` (`Parser.funcs[]`); `src/parser.c` | `PARSER_ERROR` — "too many functions (max %d)" | `FuncSig *` returned from `parser_find_function` / `parser_register_function` — used only locally within the calling function; never stored in another struct | YES | HIGH | PENDING |
| `PARSER_MAX_GLOBALS` | 256 | `include/constants.h`; `include/parser.h` (`Parser.globals[]`); `src/parser.c` | `PARSER_ERROR` — "too many global objects (max %d)" | `GlobalObjSig *` returned from `parser_find_global` — used only locally | YES | MEDIUM | PENDING |
| `PARSER_MAX_TYPEDEFS` | 64 | `include/constants.h`; `include/parser.h` (`Parser.typedefs[]`); `src/parser.c` | `PARSER_ERROR` — "too many typedefs (max %d)" | `TypedefEntry *` returned from `parser_find_typedef` — used only locally | YES | HIGH | PENDING |
| `PARSER_MAX_ENUM_CONSTS` | 256 | `include/constants.h`; `include/parser.h` (`Parser.enum_consts[]`); `src/parser.c` | `PARSER_ERROR` (enum_const overflow); no check for enum_tag overflow after registration | `EnumConst *ec` used only locally | YES | MEDIUM | PENDING |
| `PARSER_MAX_ENUM_TAGS` | 32 | `include/constants.h`; `include/parser.h` (`Parser.enum_tags[]`); `src/parser.c` | `PARSER_ERROR` — "too many enum tags (max %d)" | `EnumTag *et` used only locally | YES | LOW | PENDING |
| `PARSER_MAX_STRUCT_TAGS` | 32 | `include/constants.h`; `include/parser.h` (`Parser.struct_tags[]`); `src/parser.c` | `PARSER_ERROR` — "too many struct tags (max %d)" | `StructTag *st` returned from `parser_find_struct_tag` and `parser_register_struct_tag`; only used locally; `st->type` is updated in-place but the pointer itself is not stored | YES | MEDIUM | PENDING |
| `PARSER_MAX_UNION_TAGS` | 32 | `include/constants.h`; `include/parser.h` (`Parser.union_tags[]`); `src/parser.c` | `PARSER_ERROR` — "too many union tags (max %d)" | `UnionTag *ut` used only locally | YES | LOW | PENDING |
| `PARSER_MAX_STRUCT_FIELDS` | 64 | `include/constants.h`; `src/parser.c` lines 397, 590 — local stack array `StructField tmp_fields[]` inside struct/union body parsing | **Silent data loss** — fields beyond 64 are silently dropped (check is `if (n_fields < 64)` using a hardcoded literal instead of `PARSER_MAX_STRUCT_FIELDS`); `n_fields` still increments so struct size and alignment are also wrong. **Bug: check uses literal `64` not the constant.** | No — local stack array; contents are `memcpy`'d to a calloc'd `StructField` array after parsing | N/A (stack variable) | HIGH | PENDING |
| `FUNC_MAX_PARAMS` | 16 | `include/constants.h`; `include/parser.h` (`FuncSig.param_types[]`); `src/parser.c` | `PARSER_ERROR` — "function has %d parameters; max supported is %d" | `TypeKind` values (enum scalars), not pointers; no aliasing | NO (embedded in `FuncSig` which is embedded in `Parser.funcs[]`) | LOW | PENDING |
| `MAX_ARRAY_DIMS` | 8 | `src/parser.c` (local `#define`, not in `constants.h`) — `ParsedDeclarator.array_dims[]` and local `int dims[]` arrays | `PARSER_ERROR` — "too many array dimensions (max %d)" | No — local stack variables in `parse_declarator` and `parse_subscript_type` | N/A (stack variable) | LOW | PENDING |

---

## Code Generator

| Name | Max | Module | On Overflow | Ext Ptr Refs | Safe Realloc | Priority | Status |
|------|-----|--------|-------------|--------------|--------------|----------|--------|
| `MAX_LOCALS` | 256 | `include/constants.h`; `include/codegen.h` (`CodeGen.locals[]`); `src/codegen.c` | `compile_error` — "too many local variables" | `LocalVar *lv = codegen_find_var(...)` — returned and used locally; never stored | YES | MEDIUM | PENDING |
| `MAX_GLOBALS` | 256 | `include/constants.h`; `include/codegen.h` (`CodeGen.globals[]`); `src/codegen.c` | `compile_error` — "too many global variables (max %d)" | `GlobalVar *gv = codegen_find_global(...)` — returned and used locally; never stored | YES | MEDIUM | PENDING |
| `MAX_BREAK_DEPTH` | 32 | `include/constants.h`; `include/codegen.h` (`CodeGen.break_stack[]`); `src/codegen.c` | **No check** — `break_stack` is written at `cg->break_depth` without a bounds test before any of the four write sites (while, do-while, for, switch). Exceeding 32 nesting levels silently corrupts adjacent `CodeGen` fields. | No — accessed only by index `cg->break_depth`; no pointers into slots | YES | HIGH | PENDING |
| `MAX_SWITCH_DEPTH` | 16 | `include/constants.h`; `include/codegen.h` (`CodeGen.switch_stack[]`); `src/codegen.c` | `compile_error` — "switch nesting exceeds max depth %d" (checked before writing) | `SwitchCtx *ctx = &cg->switch_stack[...]` — local, used immediately | YES | LOW | PENDING |
| `MAX_SWITCH_LABELS` | 256 | `include/constants.h`; `include/codegen.h` (`SwitchCtx.nodes[]` and `SwitchCtx.labels[]` embedded in `SwitchCtx`); `src/codegen.c` | `compile_error` — "too many case/default labels in switch (max %d)" | `SwitchCtx.nodes[]` stores `ASTNode *` from the AST (not aliases into the array itself) | NO (arrays are embedded in `SwitchCtx` which is embedded in `CodeGen.switch_stack[]`; making them dynamic requires heap allocation inside `SwitchCtx`) | LOW | PENDING |
| `MAX_USER_LABELS` | 64 | `include/constants.h`; `include/codegen.h` (`CodeGen.user_labels[][MAX_NAME_LEN]`); `src/codegen.c` | `compile_error` — "too many labels in function (max %d)" | No — 2D `char` array; accessed by index only | NO (2D `char` array; dynamic form requires `char **` and separate allocations) | LOW | PENDING |
| `MAX_STRING_LITERALS` | 2048 | `include/constants.h`; `include/codegen.h` (`CodeGen.string_pool[]`); `src/codegen.c` | `compile_error` — "too many string literals (max %d)". **Note:** raised from 256 → 2048 in stage 92 because the compiler itself uses ~750 string-literal occurrences. | `CodeGen.string_pool[]` stores `ASTNode *` pointers from the AST; no pointers into the pool array itself escape | YES | MEDIUM | PENDING |
| `MAX_LOCAL_STATICS` | 128 | `include/constants.h`; `include/codegen.h` (`CodeGen.local_statics[]`); `src/codegen.c` | `compile_error` — "too many local static variables (max %d)" | No — accessed by index only; no escaping pointers | YES | LOW | PENDING |
| `MAX_CALL_LAYOUT_ITEMS` | 24 | `include/constants.h`; `src/codegen.c` (`CallLayout.items[]` — local variable at call sites) | **No check** — `compute_call_layout` indexes `L->items[i]` for i in 0..nargs-1 without a bounds test. Exceeding 24 arguments silently overflows the stack-local `CallLayout`. | No — local stack variable at each call site; no aliases | N/A (stack variable) | LOW | PENDING |

---

## Preprocessor

| Name | Max | Module | On Overflow | Ext Ptr Refs | Safe Realloc | Priority | Status |
|------|-----|--------|-------------|--------------|--------------|----------|--------|
| `MAX_INCLUDE_DEPTH` | 64 | `include/constants.h`; `src/preprocessor.c` (recursion depth counter) | `fprintf(stderr, …); exit(1)` — "maximum include depth exceeded" | N/A — recursion depth, not an array | N/A | LOW | PENDING |
| `MAX_COND_DEPTH` | 64 | `include/constants.h`; `src/preprocessor.c` — `CondFrame cond_stack[MAX_COND_DEPTH]` local variable inside `preprocess_internal` | `PARSER_ERROR`-style message with `exit(1)` | No — local stack array; `CondFrame *top` points in, but only within the same scope | N/A (stack variable) | LOW | PENDING |

---

## Already Dynamic (no action needed)

| Name | Notes |
|------|-------|
| `MacroTable` in `src/preprocessor.c` | Heap-allocated; grows by doubling via `realloc`. Fully dynamic since inception. |
| `GrowBuf` in `src/preprocessor.c` | Output buffer; grows by doubling via `realloc`. Fully dynamic since inception. |
| `ASTNode.children` | Was a fixed array; converted to a dynamically growing pointer array (initial capacity `AST_MAX_CHILDREN`, doubles on overflow) in stage 92. |

---

## Summary by Priority

### HIGH — fix before compiling larger programs

| Item | Risk |
|------|------|
| `MAX_BREAK_DEPTH` | Silent buffer overflow — no bounds check before writing `break_stack` entries |
| `PARSER_MAX_STRUCT_FIELDS` | Silent data loss — overflow check uses hardcoded `64` not the constant; fields beyond 64 are dropped silently |
| `PARSER_MAX_TYPEDEFS` | Only 64 slots; header-heavy code saturates this quickly |
| `PARSER_MAX_FUNCTIONS` | 256 slots; large translation units can exceed this |

### MEDIUM — raise or make dynamic before tackling large codebases

| Item | Risk |
|------|------|
| `PARSER_MAX_GLOBALS` | 256 slots; manageable but a hard error |
| `MAX_LOCALS` | 256 per function; generates a hard error |
| `MAX_GLOBALS` | 256 per translation unit (codegen side); hard error |
| `PARSER_MAX_STRUCT_TAGS` | 32 slots; medium-complexity headers can hit this |
| `MAX_STRING_LITERALS` | 2048 already high but pool is not deduplicated; large units can exhaust it |
| `PARSER_MAX_ENUM_CONSTS` | 256 slots; generous but hard error |

### LOW — can remain fixed for now

| Items |
|-------|
| `MAX_NAME_LEN`, `FUNC_TYPE_MAX_PARAMS`, `FUNC_MAX_PARAMS`, `MAX_CALL_LAYOUT_ITEMS`, `PARSER_MAX_ENUM_TAGS`, `PARSER_MAX_UNION_TAGS`, `MAX_SWITCH_DEPTH`, `MAX_SWITCH_LABELS`, `MAX_USER_LABELS`, `MAX_LOCAL_STATICS`, `MAX_INCLUDE_DEPTH`, `MAX_COND_DEPTH`, `MAX_ARRAY_DIMS` |
