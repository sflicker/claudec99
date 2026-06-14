# Stage 116 Kickoff — Global Struct Array Runtime Crashes

## Spec summary

Stage 116 is a codegen-only bug-fix stage addressing two crashes/silent data corruption issues in global struct/union arrays:

**Bug 1: BSS size wrong for single-dimension struct/union arrays**  
The BSS reserve directive emits `resd N` (N × 4 bytes) instead of `resb (N × struct_size)` for a 1-D array whose element type is a struct or union. This causes memory beyond the first 4 bytes per element to alias unrelated memory.

**Bug 2: String literals in char[N] sub-arrays emit a pointer**  
When a 2-D `char[R][C]` global array is initialized from a brace list of string-literal rows, each row emits `dq Lstr<N>` (an 8-byte pointer) instead of `C` inline bytes, corrupting the data layout.

No tokenizer, parser, or AST changes required.

---

## Tokenizer changes

None.

---

## Parser changes

None.

---

## AST changes

None.

---

## Code generation changes

### Bug 1: BSS emission fixes

- **Fix 1a:** `codegen_emit_bss()` single-dimension else-branch — change from `resd × length` to `resb × full_type->size` for all array types.
- **Fix 1b:** `codegen_emit_local_statics()` single-dimension else-branch — apply the same fix for block-scope static struct arrays.

### Bug 2: String-literal initialization for char[N] elements and fields

Define helper function `emit_string_as_bytes()` that emits string literals as inline `db` bytes with zero-padding.

- **Fix 2a:** `codegen_emit_data()` global array loop — detect `char[N]` element type and call `emit_string_as_bytes()` instead of emitting a pointer.
- **Fix 2b:** `emit_global_array_elements()` recursive helper — add `char[N]-from-string` branch before the catch-all `compile_error`.
- **Fix 2c:** `emit_global_struct()` struct field handler — add `char[N]-from-string` branch for struct fields initialized from string literals.

### Version bump

Update `src/version.c` to `VERSION_STAGE = "01160000"`.

---

## Test plan

Seven new test cases covering:
- uninitialized global struct arrays (both small and large),
- block-scope static struct arrays,
- initialized global struct arrays,
- 2-D char arrays initialized from string literals,
- struct fields of type `char[N]` initialized from string literals.

All existing tests must continue to pass.
