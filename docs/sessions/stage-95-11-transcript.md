# stage-95-11 Transcript: Convert static storages in codegen

## Summary

Stage 95-11 continues the fixed-capacity reduction work by converting five `char field[MAX_NAME_LEN]` embedded arrays in codegen.h structs to `const char *` pointers: `LocalVar.name`, `LocalVar.static_label`, `LocalStaticVar.label`, `GlobalVar.name`, and `GlobalVar.init_label`. Names from AST/lexer-owned storage are assigned as direct pointers; generated labels (`Lstatic_*` and `Lstr*`) use `util_strdup` for independent dynamic allocation. The stage also replaces the 2D array `CodeGen.user_labels[MAX_USER_LABELS][MAX_NAME_LEN]` (plus the `user_label_count` field) with `Vec user_labels; /* const char * */`, eliminating the hard 64-label-per-function cap. `MAX_USER_LABELS` is removed from `include/constants.h`. The `MAX_NAME_LEN` limit on codegen name/label fields is entirely removed; the only remaining application is `StructField.name` in `type.h`.

## Changes Made

### Step 1: LocalVar.name and LocalVar.static_label conversion

- Changed `LocalVar.name` from `char name[MAX_NAME_LEN]` to `const char *name` in include/codegen.h.
- Changed `LocalVar.static_label` from `char static_label[MAX_NAME_LEN]` to `const char *static_label` in include/codegen.h.
- Updated `codegen_register_local` in src/codegen.c: `name` assigned directly from `node->value` (lexer pool pointer); `static_label` assigned from `util_strdup(label)` since the label is generated.
- All tests pass; committed.

### Step 2: LocalStaticVar.label conversion

- Changed `LocalStaticVar.label` from `char label[MAX_NAME_LEN]` to `const char *label` in include/codegen.h.
- Simplified initialization in `codegen_register_local_static` in src/codegen.c: generate the `Lstatic_*` label, call `util_strdup`, then assign directly to `static_var.label` before `vec_push`.
- Eliminated the push-then-get-then-copy pattern; now push-then-direct-assignment.
- All tests pass; committed.

### Step 3: GlobalVar.name and GlobalVar.init_label conversion

- Changed `GlobalVar.name` from `char name[MAX_NAME_LEN]` to `const char *name` in include/codegen.h.
- Changed `GlobalVar.init_label` from `char init_label[MAX_NAME_LEN]` to `const char *init_label` in include/codegen.h.
- Updated `codegen_register_global` in src/codegen.c: `name` assigned directly from `decl->value` (lexer pool pointer); `init_label` assigned from `init->value` (direct) if present, or `util_strdup(label)` for the generated `Lstr%d` case.
- All tests pass; committed.

### Step 4: CodeGen.user_labels vector conversion

- Removed `char user_labels[MAX_USER_LABELS][MAX_NAME_LEN];` from `CodeGen` struct in include/codegen.h.
- Removed `int user_label_count;` field from `CodeGen` struct.
- Added `Vec user_labels; /* const char * */` field to `CodeGen` struct.
- Updated `codegen_init` in src/codegen.c to initialize `cg->user_labels` with `vec_init(&cg->user_labels, sizeof(const char *), 16)`.
- Updated `codegen_free` to call `vec_free(&cg->user_labels)`.
- Updated `collect_user_labels` to call `vec_push(&cg->user_labels, &node->value)` (labels are lexer-pool pointers).
- Updated per-function reset in `codegen_compile_func` to call `vec_clear(&cg->user_labels)`.
- Updated duplicate label check in `codegen_compile_func` to use `const char **pp = (const char **)vec_get(&cg->user_labels, i); const char *s = *pp;` pattern for C0 compatibility (two-statement pattern instead of inline cast-dereference).
- Updated label assembly emission in `codegen_compile_func` to use `const char **pp = (const char **)vec_get(&cg->user_labels, i); const char *s = *pp;` to retrieve each label.
- Removed capacity check (hardcoded 64-label limit no longer applies; Vec grows as needed).
- All tests pass; committed.

### Step 5: Remove MAX_USER_LABELS constant

- Removed `#define MAX_USER_LABELS 64` from include/constants.h.
- Updated `docs/fixed-capacity-inventory.md` to mark `MAX_USER_LABELS` with ✓ DONE and stage 95-11.
- Updated `MAX_NAME_LEN` entry in fixed-capacity-inventory.md to show all codegen struct fields are now pointer-based (stage 95-11).
- Updated `src/version.c` to stage 00951100.
- All tests pass; committed.

### Step 6: Documentation updates

- Updated `docs/self-compilation-report.md` to reflect stage 95-11 completions.
- Verified self-host: C0→C1→C2 all pass 1479/1479 tests without bootstrap failures.

## Final Results

All 1479 tests pass (165 unit, 828 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm). No regressions. Self-hosting verification (C0→C1→C2 cycle) confirms all stages compile and execute correctly with no bootstrap failures.

## Session Flow

1. Read spec and fixed-capacity inventory (identified 5 codegen struct fields and 1 2D array for conversion).
2. Reviewed codegen.h structs and codegen.c implementation to identify pointer-safe assignments.
3. Verified storage safety: AST/lexer names can be assigned directly; generated labels use `util_strdup`.
4. Implemented 5 field conversions (LocalVar.name, LocalVar.static_label, LocalStaticVar.label, GlobalVar.name, GlobalVar.init_label) one at a time.
5. Implemented 2D array-to-Vec conversion for user_labels with proper Vec initialization/cleanup and duplicate-check patterns.
6. Removed `MAX_USER_LABELS` constant from constants.h.
7. Updated fixed-capacity-inventory.md, version constant, and README.md.
8. Ran full test suite after each conversion commit; verified 1479/1479 pass at every stage.
9. Verified self-hosting with C0→C1→C2 cycle; all stages pass.
