# Stage 22-02 Kickoff: Global Initializers and File-Scope Declaration Validation

## Summary of the Spec

This stage extends stage-22-01's file-scope global variables by adding support for constant integer initializers. Initialized globals go to the `.data` section; uninitialized globals remain in `.bss`. Multiple declarators in one statement (e.g., `int a = 1, b, c = 3;`) are now supported at file scope.

New validation includes:
- Reject non-constant global initializers (function calls, variable references)
- Reject duplicate global object names
- Reject incompatible type redeclarations
- Reject function/object name conflicts (functions and file-scope objects share the ordinary identifier namespace)

**Known spec issues (non-blocking):**
1. Test 2: `return c+s+i+;` should be `return c+s+i+l;` (expect 10 = 1+2+3+4)
2. Heading: "initialized and initialized globals" should be "initialized and uninitialized globals"
3. Example: `char = 65;` should be `char c = 65;`

## Tokenizer Changes

None required. Existing tokenizer handles all syntax needed.

## Parser Changes

**New state in include/parser.h:**
- Add `GlobalObjSig` struct containing name and kind fields
- Add `globals[]` array (capacity MAX_GLOBALS) and `global_count` to Parser struct

**New helpers in src/parser.c:**
- `parser_find_global(const char *name)` - search global object table by name
- `parser_register_global(const char *name, TypeKind kind)` - add object to global table, reject duplicates and incompatible redeclarations

**Modifications to existing functions:**

1. `parser_register_function` - Before registering a function, call `parser_find_global` to check for function/object name conflicts. Reject if global object with same name exists.

2. `parse_external_declaration` - Handle optional initializers and validate them:
   - After parsing declarator, check for `= <constant_init>`
   - If initializer present, validate it is a compile-time constant literal (integer or pointer to 0)
   - For non-function declarators: call `parser_register_global` to register in global table
   - Check function/object conflicts before registering new global objects
   - Handle comma-separated declarator lists at file scope (return `AST_DECL_LIST`)
   - Reject function declarations with initializers

## AST Changes

No changes required. `AST_DECL_LIST` already exists from inner-scope declarations. It will now be produced at file scope as well.

## Code Generation Changes

**Modifications to include/codegen.h:**
- Add `init_value` (long) and `is_initialized` (int) fields to `GlobalVar` struct

**New helpers in src/codegen.c:**
- `data_init_directive(TypeKind)` - map type to NASM directive (db/dw/dd/dq)

**Modifications to existing functions:**

1. `codegen_add_global` - Extract initializer constant value from child AST node when present; store in `init_value` and set `is_initialized` flag.

2. `codegen_emit_bss` - Skip initialized globals (those with `is_initialized` flag set); handle `AST_DECL_LIST` at file scope.

3. Add `codegen_emit_data` - Emit `.section .data` with initialized globals in format: `name: <directive> <value>` (e.g., `x: dd 7`).

4. `codegen_translation_unit` - First pass: handle `AST_DECL_LIST` at file scope to register all globals. Call `codegen_emit_data` before or after `.text` section as appropriate.

## Test Plan

### Valid Tests (7 new)

1. **test_global_init_basic__7.c** - Basic initialized global
   ```c
   int x = 7;
   int main() { return x; }
   ```
   Expect: 7

2. **test_global_init_types__10.c** - Multiple types initialized
   ```c
   char c = 1;
   short s = 2;
   int i = 3;
   long l = 4;
   int main() { return c + s + i + l; }
   ```
   Expect: 10

3. **test_global_init_mixed__12.c** - Mix of initialized and uninitialized globals
   ```c
   int a = 5;
   int b;
   int main() {
       b = 7;
       return a + b;
   }
   ```
   Expect: 12

4. **test_global_init_multi_declarator__6.c** - Multiple declarators with selective initialization
   ```c
   int a = 1, b, c = 3;
   int main() {
       b = 2;
       return a + b + c;
   }
   ```
   Expect: 6

5. **test_global_init_counter__12.c** - Initialized global mutated through function calls
   ```c
   int counter = 10;
   int inc() {
       counter = counter + 1;
       return counter;
   }
   int main() {
       inc();
       inc();
       return counter;
   }
   ```
   Expect: 12

6. **test_global_init_shadow__4.c** - Local variable shadows initialized global
   ```c
   int x = 10;
   int main() {
       int x = 4;
       return x;
   }
   ```
   Expect: 4

7. **test_global_pointer_null_init__87.c** - Pointer initialized to 0, then assigned and dereferenced
   ```c
   int *p = 0;
   int main() {
       int x = 87;
       p = &x;
       return *p;
   }
   ```
   Expect: 87

### Invalid Tests (5 new)

1. **test_invalid_110__non_constant_initializer_function.c** - Reject function call as initializer
   ```c
   int f() { return 1; }
   int x = f();
   int main() { return x; }
   ```
   Expect: compile error

2. **test_invalid_111__non_constant_initializer_variable.c** - Reject variable reference as initializer
   ```c
   int a = 1;
   int b = a;
   int main() { return b; }
   ```
   Expect: compile error

3. **test_invalid_112__global_object_function_conflict.c** - Reject global object then function with same name
   ```c
   int f;
   int f() { return 1; }
   ```
   Expect: compile error

4. **test_invalid_113__function_object_conflict.c** - Reject function then global object with same name
   ```c
   int f();
   int f;
   int f() { return 1; }
   ```
   Expect: compile error

5. **test_invalid_114__incompatible_global_redeclaration.c** - Reject incompatible type redeclaration
   ```c
   int x;
   long x;
   ```
   Expect: compile error
