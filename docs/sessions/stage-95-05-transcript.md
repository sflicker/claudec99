# stage-95-05 Transcript: Convert Medium-Risk Static Usages

## Summary

Stage 95-05 converted six medium-risk fixed-capacity static arrays in the compiler to use the `Vec` dynamic growable-array module. The targeted arrays were in the Parser (globals, enum_consts, struct_tags) and CodeGen (locals, globals, string_pool) subsystems. Selection criteria were: Safe Realloc = YES (no external aliases), moderate usage footprint, and acceptable blast radius.

During self-hosting verification, two bootstrap defects were discovered and fixed: a parser issue where cast-of-dereference patterns like `*(ASTNode **)vec_get(...)` were mis-parsed, and a `vec_push` capacity overflow check that used signed division instead of unsigned. All 1471 tests pass at C0, C1, and C2.

## Changes Made

### Step 1: Parser globals conversion

- Replaced `GlobalObjSig globals[PARSER_MAX_GLOBALS]` and `int global_count` with `Vec globals` field in the `Parser` struct.
- Updated `parser_init()` to call `vec_init(&p->globals, sizeof(GlobalObjSig))`.
- Updated `parser_free()` to call `vec_free(&p->globals)`.
- Modified global registration to call `vec_push(&p->globals, &sig)` instead of array assignment.
- Updated global lookup loops to iterate using `vec_get(&p->globals, i)` and respect Vec length.
- Removed constant `PARSER_MAX_GLOBALS` from `include/constants.h`.

### Step 2: Parser enum_consts conversion

- Replaced `EnumConst enum_consts[PARSER_MAX_ENUM_CONSTS]` and `int enum_const_count` with `Vec enum_consts` field in the `Parser` struct.
- Updated `parser_init()` to call `vec_init(&p->enum_consts, sizeof(EnumConst))`.
- Updated `parser_free()` to call `vec_free(&p->enum_consts)`.
- Modified enum constant registration to call `vec_push(&p->enum_consts, &konst)` instead of array assignment.
- Updated enum constant lookup loops to use `vec_get()` and respect Vec length.
- Removed constant `PARSER_MAX_ENUM_CONSTS` from `include/constants.h`.

### Step 3: Parser struct_tags conversion

- Replaced `StructTag struct_tags[PARSER_MAX_STRUCT_TAGS]` and `int struct_tag_count` with `Vec struct_tags` field in the `Parser` struct.
- Updated `parser_init()` to call `vec_init(&p->struct_tags, sizeof(StructTag))`.
- Updated `parser_free()` to call `vec_free(&p->struct_tags)`.
- Modified struct tag registration to call `vec_push(&p->struct_tags, &tag)` instead of array assignment.
- Updated struct tag lookup loops to use `vec_get()` and respect Vec length.
- Removed constant `PARSER_MAX_STRUCT_TAGS` from `include/constants.h`.

### Step 4: CodeGen locals conversion

- Replaced `LocalVar locals[MAX_LOCALS]` and `int local_count` with `Vec locals` field in the `CodeGen` struct.
- Updated `codegen_init()` to call `vec_init(&cg->locals, sizeof(LocalVar))`.
- Updated `codegen_free()` to call `vec_free(&cg->locals)`.
- Modified local variable append operations to call `vec_push(&cg->locals, &var)` instead of array assignment.
- Updated all local variable lookup and iteration logic to use `vec_get()` and respect Vec length.
- Removed constant `MAX_LOCALS` from `include/constants.h`.

### Step 5: CodeGen globals conversion

- Replaced `GlobalVar globals[MAX_GLOBALS]` and `int global_var_count` with `Vec globals` field in the `CodeGen` struct.
- Updated `codegen_init()` to call `vec_init(&cg->globals, sizeof(GlobalVar))`.
- Updated `codegen_free()` to call `vec_free(&cg->globals)`.
- Modified global variable append operations to call `vec_push(&cg->globals, &var)` instead of array assignment.
- Updated all global variable lookup and iteration logic to use `vec_get()` and respect Vec length.
- Removed constant `MAX_GLOBALS` from `include/constants.h`.

### Step 6: CodeGen string_pool conversion

- Replaced `ASTNode *string_pool[MAX_STRING_LITERALS]` and `int string_pool_count` with `Vec string_pool` field in the `CodeGen` struct.
- Updated `codegen_init()` to call `vec_init(&cg->string_pool, sizeof(ASTNode*))`.
- Updated `codegen_free()` to call `vec_free(&cg->string_pool)`.
- Modified string pool append operations to call `vec_push(&cg->string_pool, &node)` instead of array assignment.
- Updated all string pool lookup and iteration logic to use `vec_get()` and respect Vec length.
- Removed constant `MAX_STRING_LITERALS` from `include/constants.h`.

### Step 7: Bootstrap defect fixes

#### Parser cast-of-dereference pattern

- Identified that C0 parser could not correctly parse cast-of-dereference patterns like `*(ASTNode **)vec_get(...)` in a single expression.
- Split problematic patterns in `src/codegen.c` and `src/parser.c` into two statements: first capturing `vec_get()` result in a temporary, then dereferencing the cast in a separate statement.
- Example fix: `*(ASTNode **)vec_get(&pool, i)` split into `ASTNode **p = vec_get(&pool, i); ASTNode *node = *p;`.

#### vec_push capacity overflow check

- Identified that `vec_push`'s overflow check in `src/vec.c` was using signed division when the divisor was a local variable, causing false overflow detection in C1.
- Rewrote overflow check to use explicit `size_t` casts to guarantee unsigned arithmetic.
- Before: `size_t divisor = v->elem_size; if (new_cap > (size_t)-1 / divisor) ...` could emit `idiv` (signed quotient).
- After: `size_t divisor = (size_t)v->elem_size; size_t max_cap = (size_t)-1 / divisor; if (new_cap > max_cap) ...` guarantees unsigned division.

### Step 8: Documentation and cleanup

- Updated `docs/fixed-capacity-inventory.md` to mark six items as DONE (stage 95-05):
  - `PARSER_MAX_GLOBALS` (512 slots) converted.
  - `PARSER_MAX_ENUM_CONSTS` (256 slots) converted.
  - `PARSER_MAX_STRUCT_TAGS` (128 slots) converted.
  - `MAX_LOCALS` (512 slots) converted.
  - `MAX_GLOBALS` (256 slots) converted.
  - `MAX_STRING_LITERALS` (256 slots) converted.
- Removed six constants from `include/constants.h`.
- Updated `docs/self-compilation-report.md` with stage 95-05 self-hosting results.

## Final Results

All 1471 tests pass at C0, C1, and C2:
- 165 unit
- 823 valid
- 237 invalid
- 82 integration
- 43 print_ast
- 100 print_tokens
- 21 print_asm

No regressions. The compiler successfully self-compiles: C0 (GCC-built) → C1 (bootstrap) → C2 (self-hosted) with identical test results at each stage.

## Session Flow

1. Read spec and fixed-capacity-inventory to identify medium-risk conversion targets.
2. Reviewed Parser and CodeGen structs to understand usage patterns and access frequencies.
3. Analyzed each target array for external aliases and access patterns (all six had Safe Realloc = YES).
4. Implemented Parser.globals Vec conversion with registration and lookup updates.
5. Implemented Parser.enum_consts Vec conversion.
6. Implemented Parser.struct_tags Vec conversion.
7. Implemented CodeGen.locals Vec conversion with all access patterns updated.
8. Implemented CodeGen.globals Vec conversion.
9. Implemented CodeGen.string_pool Vec conversion.
10. Built with cmake and ran full test suite (all tests passed at C0).
11. Ran bootstrap self-hosting build (build.sh --mode=self-host); discovered cast-of-dereference parsing issue.
12. Split problematic patterns in codegen and parser to work around C0 parser limitation.
13. Discovered vec_push overflow check using signed division with local variables; applied size_t casts for unsigned arithmetic.
14. Re-ran self-hosting cycle: C0 → C1 → C2 with all tests passing at each stage.
15. Updated inventory and documentation files.
16. Recorded final results.
