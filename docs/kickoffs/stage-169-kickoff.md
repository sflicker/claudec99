# Stage 169 Kickoff — Debug Information (DWARF)

## Summary of the Spec

Stage 169 adds DWARF debug information support to the compiler via a `-g` flag.

When `-g` is passed to `cc99`, the compiler sets a debug flag and emits NASM `%line` directives into the generated assembly before each function declaration and statement. When NASM assembles with `-g -F dwarf`, it reads these directives and produces `.debug_line`, `.debug_abbrev`, and `.debug_info` sections in the ELF object file. This allows GDB to provide source-level debugging: setting breakpoints by line, stepping through C source, and showing correct file:line information in backtraces.

The stage uses existing source position information (src_file, src_line, src_col) already carried by AST nodes since Stage 86. No lexer or parser changes required.

## Tokenizer Changes

None required. Lexer already populates AST node position fields.

## Parser Changes

None required. Parser already captures source positions during AST construction.

## AST Changes

None required. AST nodes already carry src_file, src_line, src_col.

## Code Generation Changes

### include/codegen.h

Add three new fields to the `CodeGen` struct:
- `int emit_debug` — flag indicating whether to emit %line directives
- `const char *debug_last_file` — pointer to last emitted file (for deduplication)
- `int debug_last_line` — last emitted line number (for deduplication)

### src/codegen.c

1. **codegen_init()**: Zero-initialize the three new fields (already done via memset if used; otherwise add explicit assignments).

2. **New helper emit_debug_line()**: Insert static helper after `cg_mark()` that:
   - Returns immediately if `emit_debug` is off
   - Returns if line ≤ 0 or file is NULL
   - Returns if (file, line) matches the last emitted (deduplication)
   - Otherwise emits `%line N+0 "file.c"` and updates last_file and last_line

3. **codegen_function()**: Call `emit_debug_line(cg, node->src_file, node->src_line)` just before emitting the function label.

4. **codegen_statement()**: Call `emit_debug_line(cg, node->src_file, node->src_line)` immediately after `cg_mark(node)` at the top of the function.

### src/compiler.c

1. In `main()`, add local variable: `int emit_debug = 0;`
2. In argument loop, add branch for `-g` that sets `emit_debug = 1`.
3. Update usage string to include `[-g]`.
4. Add `int emit_debug` parameter to `compile_one_file()`.
5. Set `cg.emit_debug = emit_debug;` before calling `codegen_translation_unit()`.
6. Pass `emit_debug` when calling `compile_one_file()` from `main()`.

### bin/cc99

1. **Argument parser**: Add `-g)` case that appends `-g` to `compiler_flags`.
2. **Usage block**: Add line documenting `-g` flag.
3. **After argument loop**: Detect `-g` in compiler_flags array and set `emit_debug=1` if found.
4. **NASM invocation**: Build `nasm_debug_flags` array, pass `-g -F dwarf` to NASM when `emit_debug=1`.

### test/integration/run_tests.sh

1. After `compiler_flags` array is populated from `.cflags`, detect `-g` and build parallel `nasm_debug_flags` array.
2. Pass `nasm_debug_flags` in NASM invocation (line ~130).

### src/version.c

Bump `VERSION_STAGE` to `"01690000"`.

## Test Plan

### Integration Test: test_dwarf_debug/

Create new directory `test/integration/test_dwarf_debug/` with three files:

- **test_dwarf_debug.c**: Exercises multiple functions and statements (add, mul, main).
- **test_dwarf_debug.cflags**: Contains single line: `-g`
- **test_dwarf_debug.status**: Contains single line: `42`

Verifies:
- Compiler accepts `-g` without error
- NASM assembles annotated output with `-g -F dwarf` without error
- Linked binary produces correct exit code (42)
- %line directives do not disturb instruction ordering or correctness

### Manual Verification (not automated)

After building, use:
- `readelf --debug-dump=line` to inspect `.debug_line` section and verify line mappings
- `gdb -batch -ex "b mul" -ex "run" -ex "bt"` to confirm GDB can set breakpoints and show correct source locations

## Implementation Order

1. Add emit_debug, debug_last_file, debug_last_line to CodeGen struct (include/codegen.h)
2. Verify/add zero-initialization in codegen_init (src/codegen.c)
3. Add emit_debug_line static helper (src/codegen.c)
4. Call emit_debug_line in codegen_function before label (src/codegen.c)
5. Call emit_debug_line in codegen_statement after cg_mark (src/codegen.c)
6. Add emit_debug local, -g parsing, and parameter threading (src/compiler.c)
7. Add -g parsing and NASM debug flag logic (bin/cc99)
8. Add debug flag detection and NASM invocation (test/integration/run_tests.sh)
9. Create test_dwarf_debug/ with three files
10. Run ./test/run_all_tests.sh (all tests pass)
11. Run ./build.sh --mode=self-host (C0→C1→C2 verified)
12. Bump VERSION_STAGE to "01690000" (src/version.c)

## Decisions and Ambiguities

- **Deduplication**: emit_debug_line deduplicates consecutive identical (file, line) pairs to avoid redundant directives for sub-statements sharing the same line.
- **Pointer storage**: src_file pointer is owned by lexer's file pool and outlives codegen, so storing the pointer without copying is safe.
- **Bootstrap constraint**: emit_debug_line uses only fprintf, strcmp, and integer comparisons — all available in the existing self-hosted C subset. No // comments or VLAs used.
- **No spec issues**: Spec is clear, complete, and includes exact code snippets and implementation order.
