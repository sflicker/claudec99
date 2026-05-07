# stage-22-02 Transcript: Global Initializers and File-Scope Declaration Validation

## Summary

Stage 22-02 adds support for initialized file-scope (global) scalar variables with constant integer initializers. The stage tightens validation to reject non-constant initializers (function calls, variable references), detects and rejects duplicate global object declarations, prevents type mismatches on redeclaration, and enforces that function and file-scope object names do not collide in the ordinary identifier namespace. Initialized globals are emitted to the `.data` section with their initial values; uninitialized globals remain in `.bss`. Validation occurs at parse time via a global object registry that tracks each file-scope object and function encountered.

## Changes Made

### Step 1: Parser Header — Global Registry

- Added `GlobalObjSig` struct to track file-scope object name, type signature, and declaration kind (function vs. object).
- Added `PARSER_MAX_GLOBALS` define (256) to set capacity for global object registry.
- Extended `Parser` struct with `globals[]` array and `global_count` field to maintain the registry.

### Step 2: Parser Implementation — Global Registration and Conflict Detection

- Added `parser_find_global()` helper to search the registry for a global by name.
- Added `parser_register_global()` helper to add a new global to the registry with conflict checking.
- Updated `parser_init()` to zero `global_count` on parser initialization.
- Modified `parser_register_function()` to call `parser_register_global()` for function declarations, detecting function/object name collisions at registration time.
- Extended `parse_external_declaration()` to:
  - Accept optional `= constant_expr` after declarators at file scope.
  - Support comma-separated declarator lists (e.g., `int a = 1, b, c = 3;`).
  - Validate that initializers are compile-time constant expressions (integer literals, character literals, sizeof expressions, unary/binary arithmetic on constants).
  - Call `parser_register_global()` for each declared global object, rejecting duplicates and type mismatches.
  - Prevent non-constant and non-integer initializers (e.g., function calls, variable references).

### Step 3: Codegen Header — Initialized Global Metadata

- Extended `GlobalVar` struct with `is_initialized` (int) and `init_value` (long) fields to track whether a global is initialized and its compile-time constant value.

### Step 4: Codegen Implementation — Data Emission and Global Tracking

- Modified `codegen_add_global()` to:
  - Extract the constant initializer value from an optional `AST_INITIALIZER` node.
  - Store the value in `GlobalVar.init_value` and set `is_initialized`.
  - Default uninitialized globals to zero in `init_value` but mark `is_initialized` as 0.
- Added `data_init_directive()` helper to emit the appropriate NASM directive (`.db` for char, `.dw` for short, `.dd` for int, `.dq` for long) based on type size.
- Added `codegen_emit_data()` function to iterate over the global variable registry and emit initialized globals to the `.data` section in the form `name: directive value`.
- Modified `codegen_emit_bss()` to:
  - Skip globals marked as `is_initialized`.
  - Simplify function signature (no longer takes `ASTNode*` parameter).
- Updated `codegen_translation_unit()` to:
  - Handle `AST_DECL_LIST` nodes at the top level (file scope) in the first pass, registering all globals.
  - Call `codegen_emit_data()` before `codegen_emit_bss()` to emit initialized globals first.

### Step 5: Tests — Valid Cases

Added 7 valid test cases:

1. `test_global_init_basic__7.c` — Basic initialized global: `int x = 7; return x;` expects 7.
2. `test_global_init_types__10.c` — Multiple scalar types: `char c = 1; short s = 2; int i = 3; long l = 4;` sum is 10.
3. `test_global_init_mixed__12.c` — Mix of initialized and uninitialized: `int a = 5; int b;` with b assigned in main, sum is 12.
4. `test_global_init_multi_declarator__6.c` — Multiple declarators with mixed initialization: `int a = 1, b, c = 3;` with b assigned to 2, sum is 6.
5. `test_global_init_counter__12.c` — Global persists across function calls: `int counter = 10;` incremented twice by a function, final value is 12.
6. `test_global_init_shadow__4.c` — Local variable shadows global: `int x = 10;` globally, `int x = 4;` locally, returns 4.
7. `test_global_pointer_null_init__87.c` — Pointer initialized to null: `int *p = 0;` then assigned to a local address, returns dereferenced value 87.

### Step 6: Tests — Invalid Cases

Added 5 invalid test cases:

1. `test_invalid_110__initializer_for_global.c` — Function call initializer: `int x = f();` rejected.
2. `test_invalid_111__initializer_for_global.c` — Variable reference initializer: `int b = a;` rejected.
3. `test_invalid_112__redeclared_as_a_different_kind.c` — Object then function: `int f;` followed by `int f() {...}` rejected.
4. `test_invalid_113__redeclared_as_a_different_kind.c` — Function then object: `int f();` followed by `int f;` rejected.
5. `test_invalid_114__conflicting_type_for_global.c` — Type mismatch on redeclaration: `int x;` followed by `long x;` rejected.

### Step 7: Documentation — Grammar

Updated `docs/grammar.md` to:
- Remove the blanket restriction "initialized file-scope object declarations are not yet supported".
- Document that file-scope objects may have constant integer initializers.
- Note that array initialization and non-constant initializers remain unsupported.

## Final Results

Build: Successful. All new code integrates cleanly without compiler warnings.

Tests: 657 passed, 0 failed, 657 total.
- Valid: 408 passed (401 existing + 7 new)
- Invalid: 116 passed (111 existing + 5 new)
- Print-AST: 66 passed
- Print-tokens: 46 passed
- Print-asm: 21 passed

No regressions. All existing tests continue to pass.

Example codegen output for `int x = 7; int main() { return x; }`:

```nasm
section .data
x: dd 7

section .text
global main
main:
    push rbp
    mov rbp, rsp
    mov eax, [rel x]
    mov rsp, rbp
    pop rbp
    ret
```

Example for `int a = 1, b, c = 3;`:
- `a` (initialized to 1) emitted to `.data`
- `b` (uninitialized) emitted to `.bss`
- `c` (initialized to 3) emitted to `.data`

## Session Flow

1. Read spec and supporting files (milestone and transcript templates, spec details).
2. Reviewed existing parser and codegen structure for global tracking.
3. Designed global registry struct and conflict detection logic.
4. Implemented parser changes: global registration, initializer validation, conflict detection.
5. Implemented codegen changes: storage of initializer values, data section emission.
6. Verified test cases against spec (corrected minor typos in spec test bodies).
7. Built and ran full test suite.
8. Recorded final results and generated artifacts.
