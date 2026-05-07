# Stage-22-01 Kickoff: File-Scope Uninitialized Object Declarations

## Summary of the Spec

This stage adds support for file-scope (global) uninitialized object declarations in C. The compiler will:
- Parse global declarations (int, char, short, long, int*, int[])
- Maintain a separate global symbol table
- Emit global storage in NASM .bss section (zero-initialized by default)
- Access globals via RIP-relative addressing
- Preserve local-variable shadowing (locals searched before globals)

## Tokenizer Changes

None required. Existing tokenizer handles all syntax needed.

## Parser Changes

**Current issue to fix:**
- `parse_external_declaration` in src/parser.c does not correctly handle array declarations at file scope
- Currently stores TYPE_INT instead of TYPE_ARRAY for declarations like `int arr[10];`
- Must check `d.is_array` and set appropriate type

**Required functionality:**
- Validate that external declarations are uninitialized (no initializers like `int x = 5;`)

## AST Changes

No changes required. Existing AST supports storing type information with array and pointer flags.

## Code Generation Changes

### New Structures and State

**codegen.h:**
- Add `GlobalVar` struct with fields: `name`, `size`, `kind` (TYPE_INT, TYPE_CHAR, etc.), `full_type` (for array/pointer info)
- Add to CodeGen: `globals[]` array (MAX_GLOBALS=64), `global_count`

**codegen.c - New Helper Functions:**
1. `codegen_find_global(name)` - search global symbol table
2. `emit_load_global(name, reg)` - emit RIP-relative load
3. `emit_store_global(name, reg)` - emit RIP-relative store
4. `global_var_type(name)` - return type of global variable
5. `codegen_add_global(name, kind, size)` - register global in symbol table
6. `emit_global_bss(name, size)` - emit .bss section declaration

### Code Generation Updates

**codegen_translation_unit:**
- Iterate external declarations and register globals before processing functions
- Emit .bss section with all global declarations before .text section

**emit_array_index_addr:**
- Add case for global arrays/pointers
- Use RIP-relative addressing for globals (base address via RIP-relative load)

**sizeof_type_of_expr, expr_result_type:**
- Add fallback to global symbol table when variable not found in local scope

**codegen_expression (AST_VAR_REF):**
- Check locals first (existing)
- If not found, check globals and emit RIP-relative load

**codegen_expression (AST_ASSIGNMENT):**
- Check locals first (existing)
- If not found, emit RIP-relative store to global

**codegen_expression (AST_ADDR_OF, AST_PREFIX_INC_DEC, AST_POSTFIX_INC_DEC, AST_SIZEOF_EXPR):**
- Add global fallback paths as needed

## Implementation Order

1. Fix `parse_external_declaration` to handle file-scope array declarations
2. Add GlobalVar struct and codegen state in codegen.h
3. Implement `codegen_add_global` and helper functions in codegen.c
4. Add global registration in `codegen_translation_unit`
5. Emit .bss section in `codegen_translation_unit`
6. Implement `emit_load_global` and `emit_store_global`
7. Update `codegen_expression` for AST_VAR_REF and AST_ASSIGNMENT with global fallback
8. Update other expression operations as needed
9. Run basic tests (valid and invalid cases)
10. Run print_asm tests and verify .bss layout and RIP-relative addressing

## Test Plan

### Valid Tests
- Basic global int write/read
- Local variable shadowing global
- Cross-function persistence (global modified in one function, read in another)
- Other types: char, short, long
- Pointer global: `int *p;`
- Array global: `int arr[10];`

### Print ASM Tests
- Snapshot tests for .bss section layout
- Snapshot tests for RIP-relative load/store instructions

### Invalid Tests
- Undeclared variables still fail (no false positives from global lookup)
- Invalid initializers at file scope (if supported by spec)

## Ambiguities and Decisions

**Spec typo noted (non-blocking):**
- Line 38 in spec shows `arr; resd 10` but should be `arr: resd 10` (semicolon should be colon)
- Will use correct NASM syntax with colon

**Shadowing behavior:**
- Naturally preserved by searching locals first, then globals
- No additional logic needed

**RIP-relative addressing:**
- Required by x86-64 position-independent code conventions
- Must use `[rel g]` syntax in NASM for globals

**Global initialization:**
- Currently limited to uninitialized declarations
- .bss section implicitly zero-initializes
- Future stages may add initializer support
