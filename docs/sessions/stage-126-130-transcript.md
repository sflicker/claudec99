# Stage 126–130 Transcript: Tentative Definitions, ABI Compliance, FP Constants, Block-Scope Functions, and Struct Variadic Arguments

## Summary

Five stages were implemented in a single focused session. Stages 126–129 addressed language features and parser improvements; Stage 130 implemented the most complex code-generation task: extracting structs from variadic argument lists using SysV AMD64 register classification. All 1531 tests pass; self-hosting C0→C1→C2 verified.

---

## Stage 126 — Tentative Definitions

**Problem:** C99 §6.9.2 allows multiple tentative definitions at file scope (`int x; int x = 5;`), but the compiler rejected any re-declaration with "duplicate global declaration".

**Implementation:**

The fix required both parser and codegen changes:

1. **Parser (`src/parser.c`):** Added `int is_defined` field to `GlobalObjSig` to track which declarations have initializers. Extended `parser_register_global()` with a `has_init` parameter so the parser can re-register the same global name if the existing entry is tentative (has no initializer).

2. **Codegen (`src/codegen.c`):** Added a tentative fast-path in `codegen_add_global()`. When processing a new declaration:
   - If a `GlobalVar` already exists and the new declaration has no initializer (tentative), return silently (duplicate tentative).
   - If a `GlobalVar` exists with `is_extern` and the new declaration has no initializer, clear `is_extern` (upgrade path).
   - If the new declaration has an initializer, update the existing entry in-place: copy init fields, type, linkage, and size.

This allows patterns like `extern int x; int x = 5;` (extern tentative → definite) and `int x; int x; int x = 5;` (multiple tentatives, then definition).

**Tests added:** Six valid tests covering re-declaration ordering and extern/static variations; one invalid test ensuring actual duplicate definitions still error.

---

## Stage 127 — Callee-Saved Registers r12–r15

**Problem:** SysV AMD64 ABI mandates that rbx, r12, r13, r14, r15 be callee-saved, but only rbx was being preserved.

**Implementation:**

1. **Stack layout:** Changed the stack-size formula from `+ 8` (just rbx) to `+ 40` (5 × 8 bytes). Initialized `cg->stack_offset` to 40 instead of 8, so local variables are allocated above the callee-saved slots.

2. **Prologue:** After the existing `mov [rbp - 8], rbx`, added saves for r12–r15:
   ```asm
   mov [rbp - 16], r12
   mov [rbp - 24], r13
   mov [rbp - 32], r14
   mov [rbp - 40], r15
   ```

3. **Epilogue:** In all return paths (function exit, tail call, early return), added restores:
   ```asm
   mov r12, [rbp - 16]
   mov r13, [rbp - 24]
   mov r14, [rbp - 32]
   mov r15, [rbp - 40]
   mov rbx, [rbp - 8]
   ```

**Impact:** All 21 print_asm `.expected` files required regeneration because local variable stack offsets shifted by 32 bytes (the space now reserved for r13–r15).

---

## Stage 128 — FP Constant Expressions at File Scope

**Problem:** Only single float/double literals were accepted for file-scope initializers. `double TWO_PI = 3.14159 * 2.0;` errored because the parser's global initializer validation required `AST_FLOAT_LITERAL`, not arbitrary expressions.

**Implementation:**

Added a recursive-descent FP constant evaluator in `src/parser.c`:

```c
static double eval_fp_primary(ASTNode *node);
static double eval_fp_unary(ASTNode *node);
static double eval_fp_mult(ASTNode *node);
static double eval_fp_const_expr(ASTNode *node);
```

Each function handles one precedence level (primary → unary → multiplicative → additive). The evaluators recursively descend, extract literal values from `AST_FLOAT_LITERAL` or `AST_INT_LITERAL` nodes, and perform the appropriate arithmetic operations. Unary minus and parens (via primary) are supported; division and modulo are included but only division is meaningful for FP.

The float/double global initializer path now calls `eval_fp_const_expr` instead of `parse_assignment_expression`. Once evaluated to a double, the result is formatted via `snprintf` and stored as `init_label` in the global's declaration.

**Tests added:** Four valid tests covering arithmetic (`3.14 * 2`), negation (`-3.14`), and parentheses (`(1.0 + 2.0) * 3.0`).

---

## Stage 129 — Block-Scope Function Declarations and Extern Incomplete Arrays

**Two independent features:**

### Feature 1: Block-Scope Function Declarations

**Problem:** `{ void foo(void); }` inside a compound statement errored because the parser tried to parse the parameter list as a local variable declaration.

**Implementation:**

The parser now detects a function-type declarator (identifier followed by `(` and `)`) inside a block. When detected:

1. Use a depth counter to skip the entire parameter list (balanced parens/commas).
2. Emit `AST_TYPEDEF_DECL` (a no-op for codegen).
3. Call `parser_register_function()` with param_count `-1` to indicate unknown arity.

At the call site, the arity check skips when `sig->param_count == -1`. This allows flexible calling conventions for forward-declared functions whose signatures are unknown.

**Tests added:** Two valid tests covering a simple forward declaration and a forward declaration followed by a call.

### Feature 2: Extern Incomplete Arrays

**Problem:** `extern int arr[];` errored because the parser required array declarations to have an explicit size.

**Implementation:**

1. **Parser:** Extended the `SC_EXTERN` branch to allow `has_size=1, length=0` for array declarations. This signals an incomplete type that expects a completing definition elsewhere.

2. **Codegen:** In the update path for a re-declared global, set `existing_gv->full_type = decl->full_type`. This ensures that when `emit_global_array_elements` emits the array's elements, it uses the completing definition's length, not the zero from the extern declaration.

**Tests added:** One valid test: `extern int arr[]; int arr[3] = {1, 2, 3};`.

---

## Stage 130 — `va_arg` for Struct/Union by Value

**Problem:** `va_arg(ap, struct T)` errored with "aggregate types are not supported". Extracting a struct from a variadic argument list requires applying SysV AMD64 register classification to determine whether the struct is passed via registers or the overflow stack.

### The Three-Eightbyte Classification

SysV AMD64 classifies arguments based on size and type:

- **MEMORY class** (size > 16 bytes): passed entirely on the stack.
- **Register class** (size ≤ 16 bytes): passed in GP registers, with one eightbyte per register (up to 2 registers = 2 eightbytes).

For a struct va_arg, we must check which path was taken during the call and extract accordingly.

### Implementation

**1. `compute_struct_ret_bytes` extension:**

Added a case for `AST_BUILTIN_VA_ARG` struct nodes to pre-allocate scratch stack slots, just like struct-returning function calls. This reserves space to copy the extracted struct before its address is returned to the caller.

**2. VA_ARG struct handler (AST_BUILTIN_VA_ARG):**

Implemented three-case logic based on `struct_reg_eightbytes(arg_type)`:

- **ebs == 0 (MEMORY class):** 
  - Load overflow_arg_area pointer from va_list.
  - Copy the struct (via `rep movsb`) to the scratch slot.
  - Advance overflow_arg_area by `align8(size)`.
  - Return address of scratch slot in rax.

- **ebs == 1 (1 eightbyte, size ≤ 8):**
  - Load gp_offset from va_list.
  - If `gp_offset < 48`: load from `reg_save_area + gp_offset`, copy to scratch, increment gp_offset by 8.
  - Else: load from overflow_arg_area, copy to scratch, advance overflow_arg_area by 8.
  - Return address of scratch slot in rax.

- **ebs == 2 (2 eightbytes, size 9–16):**
  - Load gp_offset; require `gp_offset ≤ 32` (room for two consecutive slots).
  - If true: load two 8-byte slots from `reg_save_area`, copy to scratch, increment gp_offset by 16.
  - Else: load two 8-byte slots from overflow_arg_area, copy to scratch, advance overflow_arg_area by `align8(size)`.
  - Return address of scratch slot in rax.

**3. `emit_struct_addr` extension:**

Added case for `AST_BUILTIN_VA_ARG` so that assignments like `struct S s = va_arg(ap, S);` work. Calls `codegen_expression` on the va_arg node (which returns the scratch address in rax) and validates that the result is a struct type.

### The Variadic Struct Argument Bug

During implementation, a critical bug surfaced: when passing a 2-eightbyte struct as a variadic argument, the caller was only loading one GP register instead of two. Root cause: `compute_call_layout` set `is_struct = p && ...`, which is always false for variadic extras (where `p == NULL`).

**Fixes applied:**

1. **`expr_result_type`:** Extended to return `TYPE_STRUCT`/`TYPE_UNION` for struct local/global variables, not just `TYPE_INT`. This ensures variadic struct arguments are classified correctly.

2. **`compute_call_layout` enhancements:** 
   - Added `CodeGen *cg` parameter so the function can look up struct full_type from symbol tables.
   - When processing a variadic extra with struct kind, look up the variable in local/global tables to get its actual size.
   - Added forward declarations for `codegen_find_var` and `codegen_find_global`.

3. **`involves_special` check:** Updated to detect struct args in addition to FP args, ensuring variadic struct calls route through the full `compute_call_layout` path.

### Tests and Edge Cases

**Tests added:**
- `test_va_arg_struct_memory__0.c`: 24-byte struct (MEMORY class).
- `test_va_arg_struct_reg1__0.c`: 8-byte struct (1 eightbyte).
- `test_va_arg_struct_reg2__0.c`: 16-byte struct (2 eightbytes).
- Moved from invalid: `test_va_arg_struct_by_value__0.c` (now compiles and passes).

All tests verified that the extracted struct values matched the passed values.

---

## Session Flow and Debugging

### Early Implementation (Stages 126–129)

Stages 126–129 were straightforward:
- Tentative definitions involved straightforward parser/codegen changes to allow re-registration.
- Callee-saved registers required updating the frame formula and prologue/epilogue.
- FP constant expressions leveraged a simple recursive evaluator.
- Block-scope functions and extern arrays required pattern detection in the parser.

### Stage 130 Main Session

The session began by reading the roadmap and understanding the struct va_arg requirements. Initial implementation of the three-case handler was correct, but testing revealed the variadic struct argument bug.

**The bug:** When compiling `grab(n, p1, p2)` where `p1` is a 16-byte struct (2 eightbytes), the caller was only moving the first register (rsi) into position, not both rsi and rdx. This meant the callee received incomplete data.

**Diagnosis:** The `compute_call_layout` path checks `is_struct = p && ...`, which is 0 for all variadic extras. Without struct classification, the layout logic treated the 16-byte struct as a 4-byte integer, allocating only one register slot.

**Solution flow:**
1. Modified `expr_result_type` to preserve struct/union kinds for struct/union variables (not just scalars).
2. Added `CodeGen *cg` parameter to `compute_call_layout` so variadic struct args could look up their full_type.
3. Updated the call sites to pass `cg`.
4. Added symbol table lookup logic in `compute_call_layout` for variadic struct classification.
5. Updated `involves_special` to detect struct args.
6. Tested all three cases (MEMORY, reg1, reg2) and verified correct struct passing.

### Final Testing

All 1531 tests passed on the first full run:
- 1251 valid (gained 5 new + 1 moved from invalid)
- 259 invalid (lost 1 moved to valid)
- 21 print_asm (regenerated for Stage 127)

Self-hosting cycle: C0 → C1 → C2 verified. No source changes needed; compiler self-compiles without modification.

---

## Key Insights

1. **Tentative definitions** clarify the parser's role in enforcing C99 semantics. The codegen just needs to handle in-place updates of already-registered globals.

2. **ABI compliance** (Stage 127) affects every function; changes to the prologue/epilogue are fundamental and require all output files to be regenerated.

3. **Constant expression evaluation** (Stage 128) can be pushed to parse time for file-scope initializers, avoiding runtime code overhead.

4. **Pattern detection** (Stage 129) in the parser allows syntactic extensions without language grammar changes. Block-scope functions and extern arrays are detected via lookahead and handled as special cases.

5. **Struct variadic arguments** (Stage 130) required integrating type information across multiple code-generation subsystems. The key insight was that `expr_result_type` needed to preserve struct kinds, and `compute_call_layout` needed access to the full struct type at classification time. This required threading the CodeGen context through a previously self-contained function, a refactor that proved essential for correctness.

All five stages completed successfully with comprehensive test coverage and self-hosting verification.
