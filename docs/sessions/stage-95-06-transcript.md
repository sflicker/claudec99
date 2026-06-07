# stage-95-06 Transcript: Convert High-Risk Static Usages

## Summary

Stage 95-06 converts four HIGH-priority fixed-capacity static arrays from the compiler's internal infrastructure to use the `Vec` dynamic growable-array module. All four arrays were identified in the fixed-capacity inventory as high-risk due to either missing bounds checks, hardcoded overflow thresholds, or insufficient capacity. The conversions eliminate two latent bugs: a silent data-loss bug in struct/union field parsing (fields beyond 64 were silently dropped due to a hardcoded constant in the overflow check), and silent buffer overflow risk in break-stack tracking (no bounds check existed). No language features were added. All 1471 tests pass at C0, C1, and C2 with no bootstrap issues.

## Changes Made

### Step 1: Parser Struct/Union Field Arrays

- Converted `StructField tmp_fields[64]` local stack array in struct body parsing (src/parser.c, ~line 394) to `Vec tmp_fields_vec`.
- Converted `StructField tmp_fields[64]` local stack array in union body parsing (src/parser.c, ~line 592) to `Vec tmp_fields_vec`.
- Fixed silent data-loss bug: the overflow check used hardcoded `64` instead of `PARSER_MAX_STRUCT_FIELDS`, so fields beyond 64 were silently dropped.
- Changed field collection pattern: `tmp_fields[count++] = field` → local `StructField` + `vec_push(&tmp_fields_vec, &field)`.
- Changed field finalization pattern: copy fields from vector to Type via `memcpy(type->fields, tmp_fields_vec.data, ...)`.
- Removed `PARSER_MAX_STRUCT_FIELDS` constant from `include/constants.h`.

### Step 2: CodeGen Break-Stack Array

- Converted `CodeGen.break_stack[MAX_BREAK_DEPTH]` (32-element array in include/codegen.h) to `Vec break_stack`.
- Added `codegen_init` initialization of `break_stack` via `vec_init`.
- Converted 4 push sites (while, do-while, for, switch statements): local `BreakFrame bf` + `vec_push(&gen->break_stack, &bf)`.
- Converted 4 pop sites: `vec_pop(&gen->break_stack)`.
- Converted 2 read sites (AST_BREAK_STATEMENT, AST_CONTINUE_STATEMENT): `vec_get(&gen->break_stack, ...)` with local pointer for access.
- Fixed silent overflow risk: no bounds check existed before any push site; Vec now enforces bounds with overflow detection.
- Removed `MAX_BREAK_DEPTH` constant from `include/constants.h`.

### Step 3: Parser Typedefs Array

- Converted `Parser.typedefs[PARSER_MAX_TYPEDEFS]` (64-element array in include/parser.h) to `Vec typedefs`.
- Updated `parser_find_typedef`: iterate via `vec_get` loop.
- Updated `parser_register_typedef`: local `TypedefEntry` + `vec_push(&parser->typedefs, &entry)`.
- Updated `parser_leave_scope`: compact in-place using two-pointer technique with `vec_get` and direct `.len` adjustment.
- Removed `PARSER_MAX_TYPEDEFS` constant from `include/constants.h`.

### Step 4: Parser Functions Array

- Converted `Parser.funcs[PARSER_MAX_FUNCTIONS]` (256-element array in include/parser.h) to `Vec funcs`.
- Updated `parser_find_function`: iterate via `vec_get` loop.
- Updated `parser_register_function`: local `FuncSig` + `vec_push(&parser->funcs, &sig)`.
- Removed now-unnecessary overflow check from `parser_register_function` (Vec bounds checking handles it).
- Removed `PARSER_MAX_FUNCTIONS` constant from `include/constants.h`.

### Step 5: Version and Documentation

- Updated src/version.c: stage bumped to `00950600`.
- Updated include/constants.h: removed all four constants.
- Updated docs/fixed-capacity-inventory.md: marked all four items as DONE.
- Updated docs/self-compilation-report.md: added stage 95-06 results with C0/C1/C2 hashes.

## Final Results

All 1471 tests pass at C0, C1, and C2 (165 unit, 823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm). No regressions. No new tests added (behavior unchanged; only internal storage modified). Self-hosting produced stable binaries with no bootstrap issues.

## Session Flow

1. Read spec and supporting files (stage 95-06 spec, milestone/transcript templates, README structure).
2. Reviewed fixed-capacity inventory to understand which four arrays were HIGH-priority targets.
3. Examined current parser.h, codegen.h, and parser.c to locate the four target arrays and their usage sites.
4. Planned phased conversions: struct fields → break-stack → typedefs → functions, one per phase.
5. Converted struct/union field arrays in src/parser.c, fixed silent data-loss bug in overflow check.
6. Converted break-stack array in CodeGen struct and all push/pop/read sites in src/codegen.c.
7. Converted Parser.typedefs and Parser.funcs arrays in include/parser.h and src/parser.c.
8. Removed four constants from include/constants.h (PARSER_MAX_STRUCT_FIELDS, MAX_BREAK_DEPTH, PARSER_MAX_TYPEDEFS, PARSER_MAX_FUNCTIONS).
9. Built and tested full suite: all 1471 tests pass at C0, C1, and C2.
10. Updated milestone, transcript, and README artifacts.
