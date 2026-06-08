# Stage 95-11 Kickoff — convert static storages in codegen

## Spec Summary

Convert all remaining `char name[MAX_NAME_LEN]`-style fixed buffers in
`codegen.h` structs to `const char *` pointers, and replace the 2D
`char user_labels[MAX_USER_LABELS][MAX_NAME_LEN]` array in `CodeGen` with
a `Vec` of `const char *`.

## Derived Stage Values

- STAGE_LABEL: `stage-95-11`
- STAGE_DISPLAY: `Stage 95-11`

## Planned Changes

| File | Change |
|------|--------|
| `include/codegen.h` | `LocalVar.name`, `LocalVar.static_label` → `const char *` |
| `include/codegen.h` | `LocalStaticVar.label` → `const char *` |
| `include/codegen.h` | `GlobalVar.name`, `GlobalVar.init_label` → `const char *` |
| `include/codegen.h` | `CodeGen.user_labels` 2D array + `user_label_count` → `Vec user_labels` |
| `include/constants.h` | Remove `MAX_USER_LABELS`; update `MAX_NAME_LEN` comment |
| `src/codegen.c` | Replace all `strncpy` with pointer assignments; `util_strdup` for generated labels |
| `src/version.c` | Update stage to `00951100` |
| `docs/fixed-capacity-inventory.md` | Mark `MAX_USER_LABELS` and codegen `MAX_NAME_LEN` fields done |

## Implementation Order

1. `LocalVar.name` + `LocalVar.static_label` — run tests
2. `LocalStaticVar.label` — run tests
3. `GlobalVar.name` + `GlobalVar.init_label` — run tests
4. `CodeGen.user_labels` Vec — run tests
5. Update `fixed-capacity-inventory.md`

## Key Decisions

- Names from AST nodes (`node->value`, `decl->value`) are persistent
  pointers into lexer-owned storage — direct assignment is safe.
- Generated labels (`Lstatic_<func>_<N>`, `Lstr<N>`) require
  `util_strdup` to persist beyond the local `char[256]` buffer.
- `static_label` and `label_ptr` (LocalVar + LocalStaticVar) share the
  same `util_strdup` result since both record the same generated label.
- No tokenizer, parser, or AST changes required.
- No new language features.
