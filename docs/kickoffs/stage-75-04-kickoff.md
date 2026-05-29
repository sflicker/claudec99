# Stage 75-04 Kickoff: va_start/va_end Codegen Foundation

## Summary of the Spec

Stage 75-04 implements code generation for `__builtin_va_start` and `__builtin_va_end` to support integer/pointer-only variadic functions on x86-64 SysV. This includes:

- Allocating a hidden 304-byte register save area in variadic function prologues
- Saving all 6 GP argument registers (rdi, rsi, rdx, rcx, r8, r9) into the save area
- Initializing the `va_list` struct fields (`gp_offset`, `fp_offset`, `overflow_arg_area`, `reg_save_area`) in `__builtin_va_start`
- Keeping `__builtin_va_end` as a no-op (unchanged from 75-03)
- Adding vfprintf, vprintf, vsnprintf declarations to test/include/stdio.h

**Spec notes corrected during planning**:
- Title typos: "fro" → "for"; "__buildin_va_end" → "__builtin_va_end"
- "unsigend int fp_offset" → "unsigned int fp_offset"
- Layout table offsets: "gp + 8" and "gp + 16" → "ap + 8" and "ap + 16"
- "vfprint" → "vfprintf"
- Test code "const * fmt" → "const char * fmt"
- Assembly comment "fbp + 16 + 8" → "rbp + 16 + 8"

## Tokenizer Changes

None required.

## Parser Changes

None required. The parser already recognizes and validates the four builtin forms from stage 75-03.

## AST Changes

None required. The AST node types for `AST_BUILTIN_VA_START`, `AST_BUILTIN_VA_END`, etc. are already defined.

## Code Generation Changes

### In include/codegen.h

Add three new fields to the `CodeGen` struct to track variadic function state:
- `variadic_reg_save_offset`: byte offset of register save area within the stack frame
- `variadic_named_gp_params`: count of named general-purpose parameters
- `variadic_named_stack_params`: count of named parameters passed on the stack (those beyond the first 6 GP slots)

### In src/codegen.c

**Function prologue changes (codegen_function)**:

When a function is variadic:
1. Add 304 to `stack_size` to reserve space for the register save area
2. Advance `stack_offset` by 304 after frame setup to reserve the area
3. Record the named parameter counts (GP params, stack params)
4. After standard frame setup (push rbp, mov rbp rsp, sub rsp stack_size), emit six register saves:
   - mov [rbp - variadic_reg_save_offset + 0], rdi
   - mov [rbp - variadic_reg_save_offset + 8], rsi
   - mov [rbp - variadic_reg_save_offset + 16], rdx
   - mov [rbp - variadic_reg_save_offset + 24], rcx
   - mov [rbp - variadic_reg_save_offset + 32], r8
   - mov [rbp - variadic_reg_save_offset + 40], r9

**va_start implementation (AST_BUILTIN_VA_START in codegen_expression)**:

Replace the no-op with actual field initialization. For `va_start(ap, last)`:
1. Load base address of `ap` (the va_list object)
2. Write `ap[0].gp_offset` = min(named_gp_params * 8, 48)
3. Write `ap[0].fp_offset` = 48
4. Write `ap[0].overflow_arg_area` = rbp + 16 + named_stack_params * 8
5. Write `ap[0].reg_save_area` = rbp - variadic_reg_save_offset

**va_end implementation (AST_BUILTIN_VA_END)**:

Remain unchanged from 75-03 (no-op, set result_type to TYPE_VOID).

## Test Plan

### Test file: test_va_start_codegen

Create an integration test that:
1. Calls a variadic function with a va_list argument from another variadic function
2. Verifies that va_start correctly initializes the va_list fields
3. Verifies that the saved register area can be accessed through reg_save_area

Example C code pattern:
```C
#include <stdarg.h>
#include <stdio.h>

void printv(const char *fmt, va_list ap) {
    vprintf(fmt, ap);
}

void print(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printv(fmt, ap);
    va_end(ap);
}

int main() {
    print("%d %d\n", 40, 2);
    return 0;
}
```

Expected output: `40 2\n`

### Test file updates: test/include/stdio.h

1. Add `#include <stdarg.h>` at the top
2. Add declarations:
   - `int vfprintf(FILE *, const char *, va_list);`
   - `int vprintf(const char *, va_list);`
   - `int vsnprintf(char *, size_t, const char *, va_list);`

## Implementation Order

1. Update `include/codegen.h` with new CodeGen struct fields
2. Update `src/codegen.c` function prologue to allocate register save area and emit register saves
3. Update `src/codegen.c` va_start implementation to initialize va_list fields
4. Update `test/include/stdio.h` to add stdarg.h include and function declarations
5. Create integration test `test_va_start_codegen`
6. Update `src/version.c` from "00750300" to "00750400"
7. Update README.md test counts and stage reference

## Ambiguities and Decisions

- **Floating-point area**: The register save area reserves 256 bytes (16 xmm registers × 16 bytes) but these are not initialized in this stage. XMM register saves are deferred to a future stage supporting floating-point variadic arguments.
- **Named parameter tracking**: The CodeGen struct must track both named GP parameters (up to 6) and named stack parameters (beyond 6) to correctly compute overflow_arg_area offset.
- **Register save offset calculation**: The register save area sits immediately below the frame pointer, so its offset is deterministic after stack_size is finalized.
- **va_list struct layout**: Follows the x86-64 SysV ABI specification (16-byte alignment, 8-byte fields except initial two 4-byte offsets).
