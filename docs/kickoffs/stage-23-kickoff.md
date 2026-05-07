# Stage 23 Kickoff: Storage Class Basics

## Spec Summary

Stage 23 introduces file-scope storage class specifiers `extern` and `static` for both objects and functions.

**Key distinctions:**
- `int x;` — definition with external linkage
- `extern int x;` — declaration only, no storage emitted
- `static int x;` — definition with internal linkage
- Same rules apply to functions; `static` functions are not marked `global` in codegen

**Extern rules:**
- Declaration without initializer does not allocate storage (no `.bss` or `.data`)
- Cannot have initializer (`extern int x = 3;` is invalid)
- Can appear before definition in same file or rely on external definition
- Mostly equivalent to ordinary function prototype for this stage

**Static rules:**
- File-scope object has internal linkage and static storage duration
- Uninitialized static objects are zero-initialized (`.bss` section)
- Initialized static objects go in `.data` section
- Static functions are not visible outside translation unit
- No `global` directive emitted in codegen for static functions

## Grammar Update

Current grammar uses `<type_specifier>` directly:
```
<function_definition> ::= <type_specifier> <declarator> <block_statement>
<declaration> ::= <type_specifier> <init_declarator_list> ";"
```

Update to insert optional storage class:
```
<function_definition> ::= <declaration_specifiers> <declarator> <block_statement>
<declaration> ::= <declaration_specifiers> <init_declarator_list> ";"
<declaration_specifiers> ::= [ <storage_class_specifier> ] <type_specifier>
<storage_class_specifier> ::= "extern" | "static"
```

Note: Spec has typo `<declaration_spedifiers>` which should be `<declaration_specifiers>`.

## Tokenizer Changes

**New tokens:**
- `TOKEN_EXTERN` for keyword `extern`
- `TOKEN_STATIC` for keyword `static`

Update `src/lexer.c` to recognize both keywords and add to keyword table.

## AST Changes

**New enum in `include/ast.h`:**
```c
typedef enum {
    SC_NONE,
    SC_EXTERN,
    SC_STATIC
} StorageClass;
```

**Modified structures:**
- Add `StorageClass storage_class` field to `GlobalObjSig` struct
- Add `StorageClass storage_class` field to `FuncSig` struct

## Parser Changes

**In `src/parser.c`:**

1. Create helper `parse_storage_class()` to consume optional `extern` or `static` token and return `StorageClass` value
2. Update `parse_external_declaration()` to call `parse_storage_class()` before parsing type
3. Pass storage class to object and function registration (e.g., `register_global_object()`, `register_function()`)
4. Update linkage conflict detection:
   - Reject `extern` then `static` (or vice versa) on same identifier
   - Reject `extern` declarations with initializers
5. Reject storage class specifiers at block scope (in `parse_declaration()` called from within blocks)
6. Maintain existing duplicate definition and object/function conflict checks

## Code Generation Changes

**In `src/codegen.c`:**

1. In global registration pass: skip storage allocation for `extern` objects (only declare without `.bss` or `.data`)
2. When emitting `global` directive for functions: only emit for non-static functions
3. Static objects and static functions remain private to translation unit

## Test Plan

**Valid tests:**
- Extern object followed by definition (both orders)
- Repeated extern declarations
- Extern function prototype
- Static global object (uninitialized)
- Static global object (initialized)
- Static function
- Static object persistence across function calls

**Invalid tests:**
- Conflicting linkage (`static` then `extern`, or vice versa)
- Extern with initializer
- Static function then non-static definition
- Duplicate static object definitions
- Block-scope static or extern (out of scope for this stage)

## Implementation Order

1. **Tokenizer** — Add TOKEN_EXTERN and TOKEN_STATIC
2. **AST** — Add StorageClass enum and storage_class field to GlobalObjSig and FuncSig
3. **Parser** — Parse storage class, update registration, validate rules
4. **Codegen** — Skip extern objects, conditional global for functions
5. **Tests** — Add test cases for all valid and invalid scenarios
6. **Grammar doc** — Update `docs/grammar.md` with new grammar rules

## Known Spec Issues

1. Grammar typo: `<declaration_spedifiers>` (line 33) should be `<declaration_specifiers>`
2. Test examples use `extern f()` and `static f()` without return type (implicit int) — implementation will require explicit `int` return type per grammar
3. Test line 230 shows `main()` without return type — implementation uses `int main()`
4. Code typos in spec examples: `countsr` (line 135), `main*()` (line 159), `/expect` (line 281)
5. Spec line 114 shows `static in x;` which appears to be typo for `static int x;`

## Key Decisions

- Storage class specifiers only supported at file scope for this stage; block-scope `static` and `extern` will be rejected
- Extern declarations do not emit storage or register as definitions
- Static objects and functions are not marked `global` in NASM output
- Implementation follows C99 rules for external linkage (`int x;`), external linkage (`extern int x;`), and internal linkage (`static int x;`)
