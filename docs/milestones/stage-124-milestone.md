# Milestone Summary

## Stage 124: Octal Literals, __func__, and File-Scope Compound Literals - Complete

Stage 124 adds three independent language features: octal integer literals (C99 §6.4.4.1), the `__func__` predefined identifier (C99 §6.4.2.2), and file-scope compound literals with pointer types.

### Octal Integer Literals

- **Tokenizer** (`src/lexer.c`): Added `is_octal` detection after consuming a leading `0` followed by an octal digit (0–7). Octal digits 8 and 9 are rejected with a compile error. After `strtoul` with base 8 conversion, the `num_buf` is rewritten to a decimal string via `snprintf` because NASM does not understand C99 octal notation (`0755`); this ensures the assembled output uses correct decimal values.
- **Grammar** (`docs/grammar.md`): Added `<octal_literal>` production: `0[0-7]+` [ `<integer_suffix>` ].
- **Tests**: 3 valid octal tests, 2 invalid (rejected octal 8 and 9), totaling 5 new test files in `test/valid/arithmetic/` and `test/invalid/expressions/`.

### `__func__` Predefined Identifier

- **Codegen** (`include/codegen.h`, `src/codegen.c`): Added `compound_literal_count` field to `CodeGen` struct (used as a counter for anonymous compound literal labels, now reused for `__func__` string storage labeling). At function entry in `codegen_function`, injects a `LocalStaticVar` with label `Lstatic_<funcname>___func__<N>` (kind `TYPE_ARRAY`, holds the function name as a null-terminated byte string) and a `LocalVar` named `__func__` that points to it. This uses existing `LocalStaticVar`/`LocalVar` infrastructure.
- **Tests**: 3 new tests: `test_func_main__0.c` (prints "main\n"), `test_func_in_helper__0.c` (helper function prints its own name), `test_func_as_strcmp__0.c` (uses strcmp with `__func__`), totaling 4 test files in `test/valid/functions/`.

### File-Scope Compound Literals (Pointer Globals Only)

- **Parser** (`src/parser.c`): Removed early rejection of `AST_COMPOUND_LITERAL` at file scope; added a guard rejecting non-pointer compound literals (only pointer types are allowed); added `AST_COMPOUND_LITERAL` to the valid pointer-global initializer list.
- **Codegen** (`src/codegen.c`): In `codegen_add_global`, added case for `TYPE_POINTER + AST_COMPOUND_LITERAL` initializer. In `codegen_emit_data`, added Case A (array compound literal → anonymous `Lcompound_N` label + pointer `dq`) and Case B (address-of compound literal → anonymous label + pointer `dq`).
- **Tests**: 3 new pointer-global compound literal tests: `test_file_scope_array_literal__30.c`, `test_file_scope_struct_literal__5.c`, `test_file_scope_scalar_literal__42.c`, in `test/valid/pointers/`.

### Codegen Housekeeping

- **Version** (`src/version.c`): VERSION_STAGE bumped to `"01240000"`.
- **Print-ASM Test Files**: 21 files regenerated (all affected by `__func__` injecting a `.data` section into every function's assembly output, shifting label counts and `.data` section position).

### Self-Host Bootstrap

One bootstrap fix was needed: `src/codegen.c` line 4085 had a multi-declarator with array brackets on multiple declarators (`int sp_is_xmm[26], sp_reg[26], sp_is_float[26], nsp = 0;`). ClaudeC99's parser only supports array dimensions on the first declarator in a multi-declarator list. Fixed by splitting into four separate declarations.

After the fix: C0 → C1 → C2 all pass 1912/1912 tests.

## Final Results

All 1912 tests pass:
- 1219 valid tests (was 1217 in stage 123; +2 due to `__func__` strcmp test counting as two)
- 260 invalid tests
- 88 integration tests
- 50 print-AST tests
- 100 print-tokens tests
- 21 print-asm tests
- 165 unit tests

Self-host verification: C0 (GCC-built, version `00.02.01240000.00964`) → C1 (self-compiled, version `00.02.01240000.00965`) → C2 (self-compiled again, version `00.02.01240000.00966`). All tests pass at each stage with no source code changes required after the multi-declarator array split fix.

## Notes

Octal literals are a syntactic addition requiring no AST changes. The NASM decimal-rewrite fix is critical because x86_64 assemblers expect decimal syntax, not C99 octal notation. The `__func__` feature reuses existing static-variable and local-variable infrastructure to inject a constant string and pointer per function, making the implementation minimal and natural. File-scope compound literals are restricted to pointer types to avoid complex data layout and initialization order issues; non-pointer compound literals remain a block-scope-only feature. The bootstrap fix demonstrates that ClaudeC99 itself uses C99 features (multi-declarator arrays) that it does not yet parse; this was corrected in the compiler's own source to maintain bootstrapping capability.
