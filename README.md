# ClaudeC99

A staged, educational C99 compiler built using Claude Code. The compiler
translates a growing subset of C99 to x86_64 assembly (NASM, ELF64, Linux).

## Pipeline

```
source.c -> Lexer -> Parser (AST) -> Code Generator -> source.asm
```

The compiler is implemented in strict ISO C99. It compiles cleanly under
both `gcc -std=c99 -pedantic-errors` and `clang -std=c99 -pedantic-errors`.
The output `.asm` file is assembled with `nasm -f elf64` and linked with
`ld -e main` to produce a Linux executable.

## Project layout

```
include/    public headers (ast, codegen, lexer, parser, type, util, token)
src/        compiler implementation
test/       test suites and runner scripts
docs/
  stages/      per-stage specifications
  milestones/  per-stage implementation summaries
  outlines/    overall checklist and overview
  grammar.md   current EBNF for the supported language
  sessions/    development session notes
  defects/     defect tracking
  kickoffs/    stage kickoff notes
status/     project status snapshots
CMakeLists.txt
```

## Build

### Normal build (GCC/Clang)

```
cmake -S . -B build
cmake --build build
```

This produces `build/ccompiler`.

### Bootstrap and self-hosting

`build.sh` wraps the build and self-hosting workflow:

```
./build.sh [--mode=MODE] [--timeout=N]
```

| Mode | What it does |
|------|--------------|
| `normal` | cmake build via GCC/Clang (default) |
| `bootstrap` | Single self-hosted build: compiles all sources with `build/ccompiler`, assembles with `nasm`, links with `gcc -no-pie`. Requires a prior normal build. |
| `self-host` | Full C0→C1→C2 cycle (self-contained): archives old named binaries to `build/previous/`, runs a fresh cmake build to produce `ccompiler-c0` (always GCC-built), bootstraps to `ccompiler-c1` (runs test suite, commits checkpoint), bootstraps to `ccompiler-c2` (runs test suite). `build/ccompiler` is left as C2. |
| `fallback` | Uses `build/ccompiler` if present, otherwise normal cmake build. |

`--timeout=N` sets the per-file compilation timeout for bootstrap builds (default: 300 s).

The standard self-hosting workflow is:

```
./build.sh --mode=self-host   # full C0→C1→C2 cycle with tests (GCC build included)
```

## Compiler limits

The remaining hard-coded capacity limits live in `include/constants.h` as
object-like macros. Redefine any of them on the compiler command line (e.g.
`-DFUNC_MAX_PARAMS=32`) or edit the file directly before building to
raise a limit.

Most former limits no longer exist: through the 95-xx cleanup stages the
parser and code-generator tables they bounded were converted to dynamic
`Vec`/`StrBuf` storage that grows on demand (so global, enum-constant, struct
tag, local, string-literal, etc. counts are no longer capped), and the
name/tag/label buffers they sized became `const char *` pointers into
lexer-owned storage (so identifier, tag, and label lengths are no longer
bounded by `MAX_NAME_LEN`, which has been removed). The macros documented
below are the only ones still in effect.

A small set of these are **intentionally kept fixed** — they are not slated for
dynamic conversion. Each one sizes an array that is embedded inside another
struct (or is a recursion-depth guard), so making it dynamic would mean a
structural refactor for limits that are generous in practice and overflow
cleanly (a diagnostic, never silent corruption). These permanent limits are
`FUNC_MAX_PARAMS` (16), `FUNC_TYPE_MAX_PARAMS` (16), `MAX_ARRAY_DIMS` (8),
`MAX_INCLUDE_DEPTH` (64), and `MAX_COND_DEPTH` (64); they are flagged
_(permanent)_ in the tables below. As of stage 95-12 the dynamic-capacity
work is complete: no candidates remain for conversion, there are no unchecked
fixed-capacity writes anywhere, and the only other tabled entries are
`AST_MAX_CHILDREN` (a doubling initial capacity, not a cap) and
`MAX_CALL_LAYOUT_ITEMS` (a guarded local-stack bound).

### AST

| Constant | Default | Description |
|----------|---------|-------------|
| `AST_MAX_CHILDREN` | 64 | Initial capacity of an `ASTNode`'s child array. The array is allocated lazily and grows by doubling, so this is a starting size rather than a hard cap (parameters in a call expression, fields in an initializer list, statements in a block, and top-level declarations in a translation unit all grow as needed). |

### Type system

| Constant | Default | Description |
|----------|---------|-------------|
| `FUNC_TYPE_MAX_PARAMS` | 16 | _(permanent)_ Maximum number of parameter types stored on a `TYPE_FUNCTION` type node (used for function-pointer types). Embedded `Type.params[]`. |

### Parser

| Constant | Default | Description |
|----------|---------|-------------|
| `FUNC_MAX_PARAMS` | 16 | _(permanent)_ Maximum number of parameters in a function declaration or definition. Embedded `FuncSig.param_types[]`. |
| `MAX_ARRAY_DIMS` | 8 | _(permanent)_ Maximum number of array dimensions in a declarator (e.g. `int a[2][3][4]` is 3). Defined in `src/parser.c` rather than `constants.h`; sizes a local/embedded `array_dims[]`. |
| `MAX_CALL_LAYOUT_ITEMS` | 24 | Maximum `ArgSlot` entries in a single call's argument layout (`FUNC_MAX_PARAMS` plus headroom for a hidden `sret` slot and variadic overhead). Local stack struct with a bounds-check guard. |

### Preprocessor

| Constant | Default | Description |
|----------|---------|-------------|
| `MAX_INCLUDE_DEPTH` | 64 | _(permanent)_ Maximum `#include` nesting depth (recursion-depth guard). |
| `MAX_COND_DEPTH` | 64 | _(permanent)_ Maximum nesting depth of `#if`/`#ifdef`/`#ifndef` conditional blocks. Embedded `cond_stack[]`. |

## Usage

```
ccompiler [--version] [--print-ast | --print-tokens] [-Werror] [--max-errors=N] [--sysroot=<dir>] [-O0|-O1|-O2] [-DNAME[=VAL]] [-I<dir>] <source.c> [<source2.c> ...]
```

- Default: writes `<name>.asm` for each source file next to the invocation directory and
  prints `compiled: <source> -> <name>.asm`.
- `--version`: prints the compiler version string and exits.
- `--print-tokens`: dumps the token stream and exits.
- `--print-ast`: dumps the AST and exits.
- `--max-errors=N`: stop after N compilation errors; `0` collects all errors
  through end of file (default: `1`).
- `--sysroot=<dir>`: sets a virtual filesystem root. Every `-I` path
  that begins with `/` is rewritten to `<dir><path>` before include
  search begins. Relative `-I` paths are left unchanged. Trailing
  slashes on `<dir>` are stripped automatically.

  Using `--sysroot=.` makes the current working directory the virtual
  root, which is useful for portable test setups that need absolute-style
  include paths without hardcoding the checkout location.

End-to-end build and run:

```
./build/ccompiler hello.c
nasm -f elf64 hello.asm -o hello.o
gcc -no-pie hello.o -o hello
./hello ; echo $?
```

`gcc -no-pie` is used in place of bare `ld` so crt0 and libc are
linked in. This lets generated programs call libc functions
(declared in the source via `int puts(char *s);` and similar) and
ensures stdio buffers are flushed at exit.

### Frontend script

`bin/cc99` wraps the three steps above into a single command with
familiar compiler-frontend conventions:

```
bin/cc99 [options] file.c [file2.c ...]
```

| Option | Effect |
|--------|--------|
| `--version` | Print compiler version and exit |
| `-o <file>` | Output file name (default: `a.out`) |
| `-c` | Compile and assemble only; produce `.o` per source file |
| `-S` | Compile only; produce `.asm` per source file |
| `-D NAME[=VAL]` | Preprocessor define (passed to `ccompiler`) |
| `-I <dir>` / `-I<dir>` | Include search path (passed to `ccompiler`) |
| `--sysroot=<dir>` | Sysroot (passed to `ccompiler`) |
| `--sysinclude` | Use system include paths instead of `test/include` stubs (Linux x86_64 only) |
| `-g` | Emit DWARF debug information (passed to `ccompiler`; enables `-g -F dwarf` in NASM) |
| `--max-errors=N` | Stop after N errors; `0` = unlimited (passed to `ccompiler`) |
| `-l <lib>` / `-llib` | Link with library (passed to `gcc`) |
| `-L <dir>` / `-Ldir` | Library search path (passed to `gcc`) |

Examples:

```
# compile and link to a.out
bin/cc99 hello.c

# specify output name
bin/cc99 -o hello hello.c

# multiple source files
bin/cc99 -o prog main.c util.c

# use project stub headers
bin/cc99 -I test/include -o prog main.c

# compile only (no link)
bin/cc99 -c hello.c          # produces hello.o
```

## Example

```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int xs[3];
    xs[0] = 10;
    xs[1] = 15;
    xs[2] = 17;

    int *p = xs;
    int sum = 0;
    for (int i = 0; i < 3; i = i + 1) {
        sum = sum + p[i];
    }
    return add(sum, 0);
}
```

## What the compiler currently supports

Through Stage 169 (DWARF debug information):

> Stage 169 adds DWARF debug information support via the `-g` flag. When compiled with `cc99 -g`, the compiler emits NASM `%line N+0 "file.c"` directives into the generated assembly before each function label and at each statement. NASM (assembled with `-g -F dwarf`) reads these directives and produces `.debug_line`, `.debug_abbrev`, and `.debug_info` ELF sections. GDB can then set breakpoints by source line, step through C source, and show correct `file:line` in backtraces. The `bin/cc99` wrapper and integration test runner both automatically pass `-g -F dwarf` to NASM when `-g` is in the compiler flags. One new integration test (`test_dwarf_debug`). All 2072 portable tests pass (165 unit, 1286 valid, 261 invalid, 189 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 189 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through Stage 168 (peephole dead-jump removal):

> Stage 168 adds the sixth built-in peephole optimization pattern: dead-jump removal. When `jmp .Lxx` is immediately followed by `Lxx:`, the jump is deleted (execution falls through anyway). Two new parse helpers (`pp_parse_jmp_label`, `pp_parse_label_def`) extract the jump target and label name; `match_dead_jump` compares them; `replace_dead_jump` keeps only the label. `g_builtin_patterns` expanded from 5 to 6 entries. Pattern fires at `-O2`. The natural trigger is a C `goto` to the immediately following label. One new integration test (`test_peephole_dead_jump`). All 2071 portable tests pass (165 unit, 1286 valid, 261 invalid, 188 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 188 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through Stage 167 (peephole redundant store elimination):

> Stage 167 adds the fifth built-in peephole optimization pattern: redundant store elimination. When two consecutive `mov [rbp - N], REG` instructions write to the same stack slot (same decimal offset, any register names), the first store is deleted and the second is kept — the first write is dead because it is overwritten before being read. The new pattern reuses `pp_parse_store_rbp` from Stage 166 without modification; `match_redundant_store` compares only offsets; `replace_redundant_store` outputs `win[1]`. `g_builtin_patterns` expanded from 4 to 5 entries. Pattern fires at `-O2`. One new integration test (`test_peephole_redundant_store`). All 2070 portable tests pass (165 unit, 1286 valid, 261 invalid, 187 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 187 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through Stage 166 (peephole redundant reload elimination):

> Stage 166 adds the fourth built-in peephole optimization pattern: redundant reload elimination. When `mov [rbp - N], REG` is immediately followed by `mov REG, [rbp - N]` (same register, same stack offset), the register already holds the value just stored — the reload is deleted and the store is preserved. Two new parse helpers (`pp_parse_store_rbp`, `pp_parse_reload_rbp`) extract the register name and decimal offset from each line form; `match_redundant_reload` verifies register and offset match across both lines; `replace_redundant_reload` passes the store through and suppresses the reload. `g_builtin_patterns` expanded from 3 to 4 entries. Pattern fires at `-O2`. One new integration test (`test_peephole_redundant_reload`). All 2069 portable tests pass (165 unit, 1286 valid, 261 invalid, 186 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 186 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through Stage 165 (peephole push/pop pair collapse):

> Stage 165 adds the third built-in peephole optimization pattern: push/pop pair collapse. When a `push rX` instruction is immediately followed by a `pop rY` instruction (adjacent lines, no intervening branch or label), the pair is collapsed to a single `mov rY, rX`, eliminating a stack memory round-trip. The special case where both registers are identical (`push rX` / `pop rX`) deletes both lines as a no-op. This is the first 2-line window pattern in the peephole optimizer (patterns 0 and 1 used single-line windows). Implementation: `pp_extract_reg` helper parses push/pop lines; `match_push_pop` verifies both lines match; `replace_push_pop` handles same-register deletion and cross-register `mov` emission with leading whitespace preserved. `g_builtin_patterns` expanded from 2 to 3 entries. Pattern fires at `-O2`. One new integration test (`test_peephole_push_pop`). All 2068 portable tests pass (165 unit, 1286 valid, 261 invalid, 185 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 185 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through Stage 164 (peephole no-op move elimination):

> Stage 164 adds the second built-in peephole optimization pattern: no-op move elimination. When a `mov REG, REG` instruction has the same register on both source and destination sides (e.g. `mov rax, rax`), the instruction is deleted entirely. The pattern fires at `-O2`, handles all register widths (64-bit `rax`, 32-bit `eax`, 16-bit `ax`, 8-bit `al`, and extended `r8`–`r15` in all widths), and uses a generic string-comparison matcher — no register lookup table needed. One new integration test (`test_peephole_nop_move`). All 2067 portable tests pass (165 unit, 1286 valid, 261 invalid, 184 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 184 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through Stage 163 (non-constant initializer for global):

> Stage 163 fixes a bug where `T *p = NULL;` at file scope was incorrectly rejected with "non-constant initializer for global" when `NULL` is defined (per GCC system stddef.h) as `((void *)0)`. The parser's pointer-global initializer validator now recognizes null pointer constant casts (`AST_CAST` from `TYPE_POINTER` containing `AST_INT_LITERAL("0")`) as valid initializers, and codegen emits `dq 0` in the data section for such globals. The fix enables proper compilation of programs using system headers where `NULL` is a cast expression rather than a bare integer literal. All 2066 portable tests pass (165 unit, 1286 valid, 261 invalid, 183 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 183 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through Stage 162 (zlib integration test):

> Stage 162 adds a zlib optional-library integration test (`test_zlib_compress`) to the `test/integration_sysinclude/` suite. The test compiles and runs the zlib compression program from the stage 158 spec: it compresses "Hello From ClaudeC99" using `compress()` from zlib and prints the original and compressed byte counts. The test is auto-skipped when zlib is not installed (`pkg-config --exists zlib` prerequisite check); it links with `-lz` and expects exit status 0. README updated to list zlib as a supported optional library. All 2065 portable tests pass (165 unit, 1286 valid, 261 invalid, 182 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 182 pass. Optional-library: 2 pass (test_sdl2_init, test_zlib_compress). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through Stage 161 (void * comparison with typed pointers):

> Stage 161 fixes a C99 compatibility bug where comparing a typed pointer (e.g., `FILE *fp`) to `NULL` was rejected with "incompatible pointer types in comparison" when using `--sysinclude`. Root cause: GCC's system `stddef.h` defines `NULL` as `((void *)0)`, making `fp == NULL` a `FILE * == void *` comparison; the stub headers define `NULL` as `0` (an integer constant), which hit a different code path and worked. The fix is one line in `src/codegen.c`: the pointer comparison validation changes from `pointer_types_equal` (requiring exact type match) to `pointer_types_assignable` (which already allows `void *` on either side per C99 §6.5.9). Two new integration tests: `test_void_ptr_cmp` (portable, directly tests `void *` vs typed pointer) and `test_null_file_cmp` (tests the exact `FILE * == NULL` scenario from the spec). All 2065 portable tests pass (165 unit, 1286 valid, 261 invalid, 182 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 182 pass. Optional-library: 1 pass (test_sdl2_init). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through Stage 160 (sizeof(expr) in constant expressions and SDL2 integration test):

> Stage 160 fixes `sizeof(expr)` in compile-time constant expression contexts (array dimensions, `case` labels, enum values, `typedef` definitions): `eval_const_primary` in `src/parser.c` now parses the operand expression, resolves struct fields from the base pointer type, and returns the correct byte size for member-access expressions like `sizeof(((SDL_Event *)NULL)->padding)`. A parallel fix in `src/codegen.c` handles the same pattern in runtime sizeof contexts (e.g., `int a = sizeof(((struct Box *)0)->label)`). New optional-library test infrastructure: `test/integration_sysinclude/` with a `run_tests.sh` runner that checks `.require` companion files and reports `SKIP` instead of `FAIL` when a prerequisite library is absent. First optional test: `test_sdl2_init` compiles and links a real SDL2 program; auto-skipped when SDL2 is not installed. Two new portable integration tests (`test_sizeof_expr_ptr`, `test_sizeof_expr_array_dim`). All 2063 portable tests pass (165 unit, 1286 valid, 261 invalid, 180 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System-include: 180 pass. Optional-library: 1 pass (test_sdl2_init). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through Stage 159 (SDL2 compile failure: GCC extension parsing fixes):

> Stage 159 fixes the SDL_main.h parse error (`expected ')', got '['`) caused by array parameters in function-pointer typedef param lists (`typedef int (*fn_t)(int argc, char *argv[]);`). Three inline parameter-type parsing loops in `parse_declarator` now handle `[]` suffixes per C99 §6.7.5.3p7, adjusting array parameters to their pointer equivalents. Five additional GCC-extension fixes allow SDL2 and other system headers to advance further through the include chain: `__attribute__((x))` and `__extension__` are predefined as empty-expansion macros; `__asm__`/`asm` statements are parsed and discarded as no-ops; anonymous struct/union members (no declarator after an inner type) are silently skipped; trailing qualifiers after the base type in function parameters (`void const *`) are consumed. One bootstrap fix: the new preamble content was merged into the existing single `builtin_preamble` string literal (cc99's char-array initializer does not support adjacent string literal concatenation). Five new integration tests. All 2239 total tests pass (2061 portable + 178 system-include). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through Stage 158 (compile failure with external library):

> Stage 158 fixes two preprocessor bugs that prevented compilation of programs using external libraries like zlib. Bug 1: `/* ... */` comments embedded in `#if`/`#elif` condition text were passed to the expression evaluator, causing division misinterpretation (root cause: zconf.h line 227 has `#if defined(__OS400__) && !defined(STDC) /* iSeries (formerly AS/400). */`). Bug 2: `#` characters inside `/* ... */` comments in macro replacement text (e.g., `#define ARG_MAX 131072 /* # bytes ... */`) triggered false "hash in object like macro" errors because validation skipped comment spans. Fixes: new `strip_cond_comments()` helper removes comments from condition text in both `#if` and `#elif` handlers; `#define` replacement validation loop now skips comment spans. Three new integration tests verify both bugs are fixed. All 2056 portable tests pass (165 unit, 1286 valid, 261 invalid, 173 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through Stage 157 (zero-register peephole pattern):

> Stage 157 ships the first peephole optimization pattern: `mov REG, 0` → `xor REG32, REG32` (2-byte encoding vs. 5–7 bytes for mov-immediate), reducing code size and improving performance. The pattern is registered in `src/peephole.c` via a matcher-replacer pair and activated at the `-O2` optimization level. Register lookup tables `g_zr_src[]` and `g_zr_dst[]` map source registers to their 32-bit counterparts; helper functions `zr_find_reg()`, `match_zero_reg()`, and `replace_zero_reg()` handle parsing and replacement across all 20 general-purpose register pairs (14 64-bit + 6 32-bit). A new `peephole_builtin_patterns()` accessor provides lazy runtime initialization for C0 compatibility. Two bootstrap issues were fixed: C0 cannot initialize structs with function-pointer fields (resolved via lazy init), and C0 mishandles `*ptr + i` where ptr is `char ***` (fixed with an intermediate variable). Four new integration tests verify correctness at `-O2` (test_zero_reg_int, test_zero_reg_long, test_zero_reg_arithmetic, test_zero_reg_logical). All 2053 portable tests pass (165 unit, 1286 valid, 261 invalid, 170 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 156 (-O1 switch dead-code removal fix):

> Stage 156 fixes a critical correctness bug in the `-O1` optimizer where switch case sections were incorrectly removed as dead code following a `break` statement (CC99-014). The bug silently broke switch-based state machines compiled with `-O1`. Root cause: the AST optimizer's `AST_BLOCK` dead-code elimination loop only stopped at `AST_LABEL_STATEMENT` but not at `AST_CASE_SECTION` or `AST_DEFAULT_SECTION`, allowing case sections following a break to be removed. One-line fix: add the two case node types to the stopping condition in `src/optimize.c`'s `optimize_stmt`. Four new integration tests verify correct behavior of switch loops under `-O0` and `-O1` (test_switch_break_o0, test_switch_break_o1, test_switch_break_default_o1, test_switch_state_update_o1). All 2049 portable tests pass (165 unit, 1286 valid, 261 invalid, 166 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 155 (peephole optimizer infrastructure):

> Stage 155 establishes the post-codegen peephole optimizer infrastructure: `include/peephole.h` and `src/peephole.c`. The peephole pass reads the emitted NASM assembly file as a line array, slides a window of 2–4 lines across it, and applies registered `PeepholeMatcher`/`PeepholeReplacer` function-pointer pairs. A new `-O2` flag wires the pass into the pipeline (at `-O2`, both the AST optimizer and the peephole pass run; no structural changes to codegen). No patterns are registered at this stage — the pass is a transparent no-op. Three new integration tests verify `-O2` produces correct output. Two bootstrap issues found and fixed: `const char * const *` in function pointer parameters (C0 does not support const-after-star in this position) simplified to `const char **`; `src/peephole.c` missing from `build.sh`'s `SRC_FILES` added. All 2045 portable tests pass (165 unit, 1286 valid, 261 invalid, 162 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 154 (unreachable statement removal):

> Stage 154 adds unreachable statement removal to the stage-142 optimizer: in `optimize_stmt`'s `AST_BLOCK` case, after each child statement is optimized, if it is a direct-child terminal statement (`return`, `break`, `continue`, or `goto`), all subsequent siblings in the same block are freed up to the next label (exclusive) and the children array is compacted. A new `is_terminal_stmt()` helper identifies the four terminal node types. Labels are never removed since they are reachable via `goto`; multiple dead zones in one block are handled by the outer loop continuing past each label. Only direct block-level children are checked — nested blocks ending in a terminal do not trigger removal of outer siblings. All gated behind `-O1`. Five new integration tests (unreachable_return, unreachable_break, unreachable_continue, unreachable_goto, unreachable_label_stop). All 2042 portable tests pass (165 unit, 1286 valid, 261 invalid, 159 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 153 (cast constant folding for scalar integer types):

> Stage 153 adds `AST_CAST` constant folding to the stage-142 optimizer: when a cast to a scalar integer type has an `AST_INT_LITERAL` operand whose numeric value fits exactly in the target type (no truncation, no sign change, no unsigned wrap), `optimize_expr` replaces the cast node with a fresh `AST_INT_LITERAL` of the target type. A new `cast_value_safe()` helper checks per-type value ranges. Unsafe casts (e.g., `(char)300` → 44, `(unsigned char)(-1)` → 255) are left for codegen. Because the optimizer walks bottom-up, `sizeof→cast→binary-fold` and `const-prop→cast→dead-branch` chains all resolve in one pass. Five new integration tests (cast_fold_basic, cast_fold_sizeof, cast_fold_const_prop, cast_fold_dead_branch, cast_fold_unsafe). All 2037 portable tests pass (165 unit, 1286 valid, 261 invalid, 154 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 152 (constant propagation for `const` scalar locals):

> Stage 152 adds constant propagation to the stage-142 optimizer: each `AST_VAR_REF` of a `const`-qualified scalar local variable initialized with an integer literal is replaced by a fresh `AST_INT_LITERAL` in `optimize_expr`. A file-static `g_const_table[64]` records eligible declarations as the optimizer walks function bodies; scope nesting is tracked by saving/restoring the table count in the `AST_BLOCK` case of `optimize_stmt`. The substituted literal is immediately visible to all downstream folding passes — binary folding (stage 143), dead-branch elimination (stage 150), etc. Only `AST_DECLARATION` nodes (not multi-var `AST_DECL_LIST`) are recorded; global consts and aggregates are excluded. Five new integration tests (const_prop_basic, const_prop_fold, const_prop_dead_branch, const_prop_scope, const_prop_init_fold), all with `.expected` output and `-O1` cflags. All 2032 portable tests pass (165 unit, 1286 valid, 261 invalid, 149 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 151 (sizeof constant folding):

> Stage 151 adds sizeof constant folding to the stage-142 optimizer: `AST_SIZEOF_TYPE` nodes are always folded to `AST_INT_LITERAL` at `-O1` (the parser stores the resolved type and size on the node); `AST_SIZEOF_EXPR` nodes are partially folded for string-literal and integer-literal operands. Two new static helpers in `optimize.c`: `sizeof_scalar_size(TypeKind)` maps type kinds to byte sizes (also fixes a latent codegen bug where `sizeof(double)` returned 4); `make_sizeof_literal(int)` produces an unsigned `TYPE_LONG` literal. After folding, sizeof values compose freely with stages 143–150 (e.g., `sizeof(long) == 8` triggers dead-branch elimination via stage 150). Five new integration tests. All 2027 portable tests pass (165 unit, 1286 valid, 261 invalid, 144 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 150 (dead-branch elimination):

> Stage 150 extends the stage-142 optimizer with dead-branch elimination for `if`, `while`, and `for` statements whose controlling condition is a compile-time integer constant. When the condition folds to an `AST_INT_LITERAL` after child optimization, the unreachable code path is freed and only the live path (or an empty block) is returned. Three new rules in `optimize_stmt`: `if(nonzero)` → then-branch, `if(0)` → else-branch or empty block, `while(0)` → empty block (non-zero while not eliminated to preserve infinite loops), `for(init;0;update)` → init only or empty block. All gated behind `-O1`. Memory management nulls dead-child slots before `ast_free` to prevent double-free. No grammar changes. Six new integration tests (test_dead_if_true, test_dead_if_false, test_dead_while, test_dead_for, test_dead_for_no_init, test_dead_combined). All 2022 portable tests pass (165 unit, 1286 valid, 261 invalid, 139 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 149 (conditional expression folding):

> Stage 149 extends the stage-142 optimizer with conditional expression folding: when the condition of a ternary operator (`?:`) is a compile-time integer constant, the entire expression is replaced with the selected branch. The condition child (condition of `?:`) may have been folded to an `AST_INT_LITERAL` by stage-143 (constant binary folding), stage-144 (constant unary folding), or earlier optimizer rules. A new rule block in `optimize_expr` detects `AST_CONDITIONAL_EXPR` nodes whose condition child is an `AST_INT_LITERAL`, selects the appropriate branch (true branch for nonzero, false branch for zero), and frees the dead branch and ternary node via `ast_free`. Memory management nulls the kept branch's child slot before freeing to prevent double-free. No grammar changes. Four new integration tests (test_cond_fold_true, test_cond_fold_false, test_cond_fold_nested, test_cond_fold_combined). All 2016 portable tests pass (165 unit, 1286 valid, 261 invalid, 133 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 148 (negation folding):

> Stage 148 extends the stage-142 optimizer with negation folding: reducing `-(-x)` to `x` for non-constant expressions. This is the arithmetic analogue of the `!!x` (double logical NOT) rule added in Stage 147. A new rule block in `optimize_expr` detects double unary-minus chains with non-literal operands and returns the inner operand directly. When the operand is a compile-time integer literal, stage-144's two-pass constant unary folding already reduces `-(-const)` to the original value, so this rule fires only for non-constant cases. Memory management nulls the inner node's child slot before freeing to prevent double-free. No grammar changes. Three new integration tests (test_neg_fold_double_minus, test_neg_fold_triple_minus, test_neg_fold_combined). All 2012 portable tests pass (165 unit, 1286 valid, 261 invalid, 129 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 147 (boolean and logical simplification):

> Stage 147 adds boolean and logical simplification to the stage-142 optimizer: `!!x → (x != 0)`, `x && 0 → 0`, `x || nonzero → 1`, `x && nonzero → (x != 0)`, and `x || 0 → (x != 0)`. Two rule blocks fire under `-O1` in `optimize_expr`: the double-NOT block (after stage-144 unary folding) and the binary boolean block (after stage-146 strength reduction). The constant case `!!const` is skipped (already handled by stage-144). Left-operand-literal cases (`0 && x`, `nonzero || x`) are already handled by stage-143 short-circuit folding and are not re-implemented. No grammar changes. Six new integration tests (bool_simplify_and_zero, bool_simplify_or_nonzero, bool_simplify_and_one, bool_simplify_or_zero, bool_double_not, bool_simplify_combined). All 2009 portable tests pass (165 unit, 1286 valid, 261 invalid, 126 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 146 (strength reduction: power-of-two multiply/divide):

> Stage 146 adds strength reduction to the stage-142 optimizer: `x * 2^N` is rewritten to `x << N` and `x / 2^N` is rewritten to `x >> N` for unsigned or statically non-negative constant dividends. Three rules fire under `-O1` in `optimize_expr` (after the stage-145 algebraic identity block): the right-operand multiply form, the commutative left-operand multiply form, and the division form. All three mutate the `AST_BINARY_OP` node in place — freeing the old power-of-two literal, installing a new shift-amount `AST_INT_LITERAL`, and updating `node->value` to the new operator. `* 1` and `/ 1` (2^0) are already handled by stage-145 identity rules; this block only fires for N ≥ 1. No grammar changes. Five new integration tests (mul_pow2, mul_pow2_commutative, div_pow2_unsigned, no_signed_div, combined). All 2003 portable tests pass (165 unit, 1286 valid, 261 invalid, 120 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 145 (algebraic identity folding):

> Stage 145 extends the stage-142 optimizer with algebraic identity rules in `optimize_expr`: rules that fire when only one operand is a constant integer, or when both operands are the same `AST_VAR_REF`. Identity rules (`x+0→x`, `0+x→x`, `x-0→x`, `x*1→x`, `1*x→x`, `x/1→x`, `x|0→x`, `0|x→x`, `x&~0→x`) null the kept child's slot before `ast_free` to avoid double-free. Zero rules (`x*0→0`, `0*x→0`, `0/x→0`, `x&0→0`, `0&x→0`, `x-x→0`, `x^x→0`) free the entire subtree and return a fresh `AST_INT_LITERAL`. Self-cancellation (`x-x`, `x^x`) is detected only for `AST_VAR_REF` nodes with the same `value` field (deliberately shallow). All folding gated behind `-O1`; `-O0` unaffected. No grammar changes. Six new integration tests. All 1998 portable tests pass (165 unit, 1286 valid, 261 invalid, 115 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 144 (constant unary folding):

> Stage 144 completes constant unary folding in the stage-142 optimizer infrastructure: `AST_UNARY_OP` nodes whose single child is an `AST_INT_LITERAL` are now folded for all four unary operators under `-O1`. The stage-143 `~`-only block in `optimize_expr()` is replaced with a unified rule: `-val` (arithmetic negation), `+val` (unary plus, identity), `!val` (logical NOT, produces `TYPE_INT` 0 or 1), and `~val` (bitwise complement). Operators applied to non-constant operands are unaffected. No grammar changes. Four new integration tests (unary_minus, unary_plus, unary_not, unary_combined). All 1992 portable tests pass (165 unit, 1286 valid, 261 invalid, 109 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 143 (constant integer binary folding):

> Stage 143 implements the first real optimization rule in the stage-142 infrastructure: **constant integer binary folding**. When both children of an `AST_BINARY_OP` are `AST_INT_LITERAL`, the optimizer computes the result at compile time and replaces the entire node with a single `AST_INT_LITERAL`. Arithmetic (`+`, `-`, `*`, `/`, `%`), bitwise (`&`, `|`, `^`, `<<`, `>>`), relational (`<`, `<=`, `>`, `>=`, `==`, `!=`), and logical short-circuit (`&&`, `||`) operators are folded; unary bitwise-NOT (`~`) on constants is also folded. Division and modulo by zero are skipped (node left unfolded for codegen to handle). Result types follow C99: relational/logical produce `TYPE_INT` with value 0 or 1; arithmetic/bitwise results inherit type from left operand. All folding is gated behind `-O1`; `-O0` path unaffected. No grammar changes. Six new integration tests. All 1988 portable tests pass (165 unit, 1286 valid, 261 invalid, 105 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 142 (optimizer infrastructure: `-O0`/`-O1` flags and AST-level optimizer scaffolding):

> Stage 142 introduces the AST-level optimizer scaffolding: a new `optimize.h` / `optimize.c` module with `optimize_translation_unit(ASTNode *root, int opt_level)` entry point and recursive `optimize_expr()` / `optimize_stmt()` helpers that walk the AST bottom-up and return every node unchanged (pure no-op infrastructure). Subsequent stages (143+) will add constant-folding and dead-branch-elimination rules on top of this skeleton. Two new flags are wired end-to-end: `-O0` (default, skip optimizer) and `-O1` (enable optimizer), accepted by both `ccompiler` and the `bin/cc99` wrapper. All 1982 portable tests pass (165 unit, 1286 valid, 261 invalid, 99 integration, 50 print-AST, 100 print-tokens, 21 print-asm). System include suite: 99/99 pass. Self-host C0→C1→C2 verified.

Through stage 139 (preprocessor `#if` expression gaps: integer suffixes, function-like macros, ternary):

> Stage 139 fixes three deficiencies in the `#if`/`#elif` constant-expression evaluator (`eval_cond_primary` and friends in `src/preprocessor.c`) that prevented parsing system headers. **PP-01 (integer literal suffixes)**: `eval_cond_primary` now consumes trailing `u`/`U`/`l`/`L` suffix characters after integer literals and parses hex literals (`0x`/`0X` prefix). **PP-02 (function-like macros in `#if`)**: the `#if` and `#elif` handlers now collect the raw condition text and pass it through `expand_macros_text()` before evaluation, implementing C99 §6.10.1 macro replacement; `expand_macros_text()` gains a special case to pass `defined(X)` / `defined X` through unexpanded; the function-like macro guard in `eval_cond_primary` changed from `exit(1)` to `return 0L`. **PP-03 (ternary operator)**: `eval_cond_ternary` added between `eval_cond_logical_or` and `eval_cond_expr`, enabling `condition ? then : else` in `#if` expressions. 9 new integration tests. Manual system-header validation script `test/integration/run_tests_sysinclude.sh` added. Version bumped to 13900000. All 1979 tests pass (1284 valid, 262 invalid, 97 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with all tests passing at every stage, no source changes needed during bootstrap.

Through stage 138 (auto and register storage-class specifiers):

> Stage 138 adds `auto` and `register` storage-class specifiers (CC99-011 and CC99-012). **Tokenizer**: `TOKEN_AUTO` and `TOKEN_REGISTER` added. **AST**: `SC_AUTO=8` and `SC_REGISTER=16` added to `StorageClass` enum. **Parser**: `auto` at block scope treated as default automatic storage; `register` at block scope allocates identically but marks the variable `SC_REGISTER`; both are rejected at file scope; `register` is also accepted as a leading qualifier in function parameter declarations. **Codegen**: `is_register` field added to `LocalVar`; `AST_ADDR_OF` rejects `&register_var` with a compile error. **Tests**: 5 new tests (2 valid returning 27, 3 invalid). Version bumped to 13800000. All 1970 tests pass (1284 valid, 262 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with all tests passing at every stage, no source changes needed during bootstrap.

Through stage 137 (functions returning function pointers):

> Stage 137 fixes the parser to accept functions returning function pointers — the `int (*get_adder())(int)` declarator form (CC99-010), previously rejected with "functions returning function pointers are not supported". **Parser**: `ParsedDeclarator` extended with `is_func_returning_fp`, `own_param_types[FUNC_TYPE_MAX_PARAMS]`, `own_param_count`, `own_is_no_prototype` fields to track the inner function's signature. `parse_declarator` replaces the rejection with full parsing of `(*name())(params)` form (guard on `inner_stars == 0` remains for the invalid direct-function-return case). **Semantic**: `parse_external_declaration` builds the nested pointer-to-function type, creates `AST_FUNCTION_DECL` with `decl_type = TYPE_POINTER`, and registers/optionally parses the function body. **Bug fix**: typedef'd pointer return types recognized via `func->full_type` assignment condition. **Tests**: removed 1 test (now valid), added 4 valid tests and 1 invalid test. Version bumped to 13700000. All 1965 tests pass (1282 valid, 259 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with all tests passing at every stage, no source changes needed during bootstrap.

Through stage 136 (sizeof of pointer-arithmetic expressions):

> Stage 136 fixes a bug in `sizeof_type_of_expr` where `sizeof` applied to pointer-arithmetic expressions (ptr+int, arr+int, ptr-ptr, string_lit+int) returns the element size instead of the pointer/ptrdiff_t size (8 on LP64). Two targeted fixes in `src/codegen.c`: (1) added a pointer/array guard in the `AST_BINARY_OP` case for `+`/`-` operators that returns `TYPE_POINTER` when either operand is `TYPE_POINTER` or `TYPE_ARRAY`, placed after the FP check but before the integer promotion path; (2) added an `AST_STRING_LITERAL` case returning `TYPE_POINTER` so string literals in binary expressions are treated as pointers and decay correctly. Version bumped to 13600000. All 1961 tests pass (1277 valid, 259 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with all tests passing at every stage, no source changes needed during bootstrap.

Through stage 135 (type compatibility and composite type checks):

> Stage 135 fixes two C99 type compatibility bugs. **CC99-008 (array parameter adjustment)**: `int a[3]`, `int a[]`, and `int *a` as function parameters are now compatible per C99 §6.7.5.3p7 — `parse_parameter_declaration` now applies the array-to-pointer adjustment for named array declarators (`d.is_array`). Function-type parameters (`int cb(void)`) are also adjusted to pointer-to-function per §6.7.5.3p8. A pre-existing bug where `(void)` in a function-pointer parameter list was counted as one void parameter instead of zero is also fixed. **CC99-009 (pointer-to-array types)**: `int (*row)[4]` and `int (*row)[]` are now valid parameter declarations. `ParsedDeclarator` gains three new fields (`is_ptr_to_array`, `ptr_to_array_length`, `ptr_to_array_has_size`); `parse_declarator` parses the `[N]` bound instead of rejecting it; `parse_parameter_declaration` builds `pointer(array(base, N))`. Composite type rule: `int (*row)[]` and `int (*row)[4]` both produce `TYPE_POINTER` and are compatible. Indexed access `(*row)[i]` works via the existing codegen path. The former invalid test for `int (*p)[10]` is moved to valid since it is valid C99. All 1951 tests pass (1267 valid, 259 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with all tests passing at every stage, no source changes needed during bootstrap.

Through stage 131 (sizeof unsigned size_t):

> Stage 131 fixes `sizeof` to produce an unsigned `size_t` result per C99 §6.5.3.4. The compiler previously treated `sizeof` as signed `long`, causing incorrect expression evaluation in three scenarios: `sizeof(int) > -1` evaluated true (wrong — under unsigned UAC, -1 converts to ULONG_MAX which is > sizeof(int), so the comparison should be FALSE and not add 1), `sizeof(char) - 2 > 0` evaluated false (wrong — under unsigned arithmetic, 1-2 wraps to ULONG_MAX-1 which is > 0, should be TRUE and add 2), and `(sizeof(int) < 0) == 0` was TRUE but coincidentally (no unsigned value can be < 0). The fix is two lines in `src/codegen.c`: set `node->is_unsigned = 1` on both `AST_SIZEOF_TYPE` and `AST_SIZEOF_EXPR` nodes when their value is materialized. The existing UAC infrastructure in `uac_is_unsigned()` and the comparison/arithmetic codegen then correctly selects unsigned instructions when a sizeof result is one of the operands. No tokenizer, parser, or AST changes were needed. One bootstrap issue was fixed: `strtod` was missing from `test/include/stdlib.h` (it was added to `src/parser.c` in stages 126-130 for FP constant expression evaluation). All 1935 tests pass (1252 valid, 259 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with all tests passing at every stage.

Through stage 124 (octal integer literals, `__func__`, file-scope compound literals):

> Stage 124 adds three independent C99 language features. **Octal integer literals** (`0NNN`): the lexer detects a leading `0` followed by digits, reads the octal digits (rejecting `8` or `9` with a compile error), converts the value via `strtoul` with base 8, then rewrites the token as a decimal string before emission — necessary because NASM does not understand C octal notation. **`__func__` predefined identifier** (C99 §6.4.2.2): at the start of every function, the codegen injects a `static const char __func__[] = "funcname"` anonymous object using the existing `LocalStaticVar`/`LocalVar` infrastructure; the variable is visible in the function body as a const char array. **File-scope compound literals for pointer globals**: `int *p = (int[]){1,2,3}` and `struct S *q = &(struct S){3,5}` are now accepted at file scope for pointer-typed globals; the parser admits `AST_COMPOUND_LITERAL` as a valid pointer initializer and codegen emits an anonymous `Lcompound_N` static object followed by a `dq` pointer. Non-pointer compound literals at file scope remain an error. One bootstrap fix was needed: `int sp_is_xmm[26], sp_reg[26], sp_is_float[26], nsp = 0;` (multi-declarator with array dimensions, introduced in stage 123) was split into four separate declarations. All 1912 tests pass (1226 valid, 262 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified.

Through stage 123 (ABI bug fixes: FP stack args, indirect FP calls, address-constant initializers, FP short-circuit):

> Stage 123 fixes five correctness bugs reported in `docs/defects/CC99_FIX_REPORT-02.md`. **CC99-001/002**: A new `is_fp` field added to `ArgSlot` disambiguates floating-point stack-overflow arguments from GP stack arguments; previously both had `xmm_idx == -1` so Phase 1 of the direct-call emission loop used `mov [rsp + off], rax` for all memory-class arguments, writing integer register bits instead of IEEE 754 values for the 9th+ `double`. The same bug affected variadic callee-reads via `va_arg(ap, double)` on the 9th+ double argument. Now FP stack args use `movsd`/`movss`. **CC99-003**: The `AST_INDIRECT_CALL` handler only had GP-register paths; XMM registers were never loaded for function-pointer calls. A new FP-aware path mirrors the direct-call Phase 1/Phase 2 logic: `compute_call_layout` classifies arguments, Phase 1 writes memory-class args with `movss`/`movsd`, Phase 2 spills then restores register-class args into `xmm0`–`xmm7` or GP registers before `call r10`. **CC99-004**: Global pointer initializers with `&global` or `&global[N]` forms were rejected by both the parser (which only accepted integer/char/string literals and bare `AST_VAR_REF` function designators) and codegen (which had no `AST_ADDR_OF` emission path). Parser validation now accepts `AST_ADDR_OF`; `codegen_emit_data` emits `dq label` or `dq label + offset`; block-scope static pointers bypass `eval_const_init` for address-constant initializers, using two new `LocalStaticVar` fields (`is_label_init`, `init_label`). **CC99-005**: The `&&` and `||` handlers skipped calling `emit_fp_bool_to_rax` before the short-circuit branch when the left operand was FP, so the branch used raw XMM bits instead of a boolean value and both sides always evaluated. Now the branch is correctly conditioned on the FP truth value. No tokenizer or AST changes. Seven new tests added. All 1901 tests pass (1217 valid, 260 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with no source changes during bootstrap.

Through stage 122 (ABI callee-saved register preservation):

> Stage 122 fixes the generated code to preserve the `rbx` register across function calls, as required by the SysV AMD64 ABI (§3.2.1: `rbx` is callee-saved). The compiler uses `rbx` as a scratch register for address arithmetic (array indexing, struct member access, lvalue increment/decrement) via balanced push/pop pairs within expression subtrees — but without a save/restore in the function prologue/epilogue, external callers (e.g., glibc `qsort`) that rely on `rbx` being preserved across a call to our generated functions suffered silent data corruption. The fix reserves a dedicated 8-byte slot at `[rbp - 8]` in every function's stack frame: `cg->stack_offset` now starts at 8, `stack_size` adds +8 for the slot, variadic functions add an extra +8 alignment pad before the 176-byte register save area (so XMM `movaps` slots remain 16-byte aligned), and each of the four epilogue paths restores rbx before `mov rsp, rbp`. All 21 `test/print_asm/` expected files were regenerated to reflect the new instructions and shifted local-variable offsets. No tokenizer, parser, or AST changes. Two new `qsort`-based callback tests added. All 1894 tests pass (1210 valid, 260 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with no source changes during bootstrap.

Through stage 121 (switch on long/long long discriminants):

> Stage 121 fixes the `switch` statement so that `long`, `long long`, and `unsigned long long` discriminants are compared with 64-bit instructions. Previously the dispatch loop emitted `mov eax, [rsp]` (32-bit load), truncating the top 32 bits of 64-bit values and causing incorrect case matching. The fix captures `disc_kind = node->children[0]->result_type` after `codegen_expression` and emits `mov rax, [rsp]` / `cmp rax, <val>` for 64-bit types; `int`, `char`, and `short` discriminants continue to use the 32-bit path (they are integer-promoted before evaluation). No tokenizer, parser, or AST changes. All 1892 tests pass (1208 valid, 260 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with no source changes during bootstrap.

Through stage 120 (FP increment/decrement on struct members):

> Stage 120 adds support for prefix and postfix `++`/`--` operators on floating-point (`float`/`double`) struct fields accessed via dot (`.`) and arrow (`->`) operators. Two codegen bugs in `codegen_inc_dec_general` were fixed: Bug 1: `TYPE_DOUBLE` fell to `default: sz = 4` in the fallback size switch (should be 8 bytes). Bug 2: integer `add`/`sub` instructions were used regardless of FP type. The fix adds an FP early-return path using SSE2 instructions (`movsd`/`movss`, `addsd`/`subsd`, `subsd`/`subss`) and two new `.rodata` constants (`Lfp_one_f64` and `Lfp_one_f32`) for the 1.0 increment/decrement operand. Postfix forms save the old value in `xmm1` before the operation. Bonus fixes: the same path also corrects `++`/`--` on FP array elements and FP pointer dereferences. No tokenizer, parser, or AST changes. All 1886 tests pass (1202 valid, 260 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with no source changes during bootstrap.

Through stage 119 (FP compound assignment on struct members):

> Stage 119 fixes compound assignment on floating-point struct fields when both operands are members of file-scope (global) struct variables. Five bugs in `expr_result_type()` and `sizeof_type_of_expr()` were missing global-variable fallbacks, causing the FP arithmetic path to be silently skipped and integer instructions to run on IEEE 754 bit patterns. A sixth related bug in `emit_arrow_addr()` prevented `gp->field` from working as an lvalue for global pointer-to-struct variables. No tokenizer, parser, or AST changes. All 1879 tests pass (1195 valid, 260 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with no source changes during bootstrap.

Through stage 118 (pointer relational operators):

> Stage 118 adds support for the four pointer relational comparison operators (`<`, `<=`, `>`, `>=`) when applied to pointer operands. Previously these operators fell to a compile error; now they are handled by extending `is_pointer_cmp` in `codegen_expression()` to cover all six comparison operators, adding a validation guard that rejects `pointer < integer` (only `==`/`!=` against the null pointer constant are valid per C99), and selecting unsigned `setb`/`setbe`/`seta`/`setae` setcc variants (pointer addresses are unsigned 64-bit values). No tokenizer, parser, or AST changes. All 1872 tests pass (1188 valid, 260 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with no source changes during bootstrap.

Through stage 117 (FP struct member rvalue reads):

> Stage 117 fixes three codegen bugs affecting floating-point struct and union field reads. Bug 1: the rvalue load instruction for `double`/`float` struct fields was `mov eax, [rax]` (4-byte integer) instead of `movsd xmm0, [rax]` / `movss xmm0, [rax]`. Fixed in both `codegen_expression()` AST_MEMBER_ACCESS and AST_ARROW_ACCESS rvalue blocks by adding FP early-return branches before the `int sz = ...` calculation. Affects all three member-access forms: direct dot (`s.x`), arrow (`p->x`), and subscript-then-dot (`arr[i].x`). Bug 2: `expr_result_type()` reported TYPE_INT for FP struct fields, causing binary arithmetic on struct fields (`a.x - b.x`) to incorrectly use the integer code path. Fixed by adding `else if (type_is_fp(f->kind)) { t = f->kind; }` branches to the AST_MEMBER_ACCESS (VAR_REF and DEREF bases) and AST_ARROW_ACCESS cases. Bug 3: `expr_result_type()` and `sizeof_type_of_expr()` fell through to TYPE_INT when the base of a member access was an AST_ARRAY_INDEX node (`bodies[j].x`). Fixed by adding a new AST_ARRAY_INDEX base handler that walks to the array variable, resolves the element type, finds the named field, and returns the correct FP/pointer/int kind. No tokenizer, parser, or AST changes. All 1863 tests pass (1181 valid, 258 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm, 165 unit). Self-host C0→C1→C2 verified with no source changes during bootstrap.

Through stage 116 (global struct array BSS fix and char[N] string-literal initialization):

> Stage 116 fixes two codegen bugs affecting global struct arrays. Bug 1: uninitialized single-dimension struct/union arrays emitted `resd N` (N × 4 bytes) in BSS instead of `resb (N × struct_size)`, causing data corruption for structs larger than 4 bytes. Fixed in both `codegen_emit_bss()` and `codegen_emit_local_statics()` by using `resb full_type->size` directly. As a side effect, all single-dimension BSS arrays now emit `resb total` uniformly. Bug 2: string literals used to initialize `char[N]` sub-arrays or struct fields emitted an 8-byte pointer (`dq LstrN`) instead of inline bytes. Fixed by adding an `emit_string_as_bytes()` helper and wiring it into three emission sites: `codegen_emit_data()` global array loop, `emit_global_array_elements()` recursive helper, and `emit_global_struct()` field emitter. No tokenizer, parser, or AST changes. All 1857 tests pass (1175 valid, 258 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm, 165 unit). Self-host C0→C1→C2 verified with no source changes during bootstrap.

Through stage 115 (constant expressions in array dimension bounds):

> Stage 115 extends the array-dimension parser to accept full C99 integer constant expressions (arithmetic, bitwise, shift, relational, equality, logical AND/OR, ternary, `sizeof`, parentheses, enum constants, macro identifiers) instead of requiring bare integer literals. Four sites in `src/parser.c` now call `eval_const_expr()` instead of checking for `TOKEN_INT_LITERAL` only: `parse_type_name` bracket loop for `sizeof(int[N])` and compound literals, `parse_direct_declarator` parenthesized form for `int (*a)[N]`, non-paren first dimension for `int a[N]`, and non-paren additional dimensions for second+ dims in multidimensional arrays. No AST or codegen changes; dimensions are evaluated at parse time via the existing evaluator (available since stage 99). The compiler's own source uses only literal constants in array dimensions, so bootstrap paths are unaffected. All 1850 tests pass (1168 valid, 258 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm, 165 unit). Self-host C0→C1→C2 verified with no source changes during bootstrap.

Through stage 114 (legacy test fixes: FP array subscript, nested brace init, mixed FP/int ternary, string literal subscript):

> Stage 114 resolves 24 failing tests from an imported legacy project and migrates 219 legacy tests to the appropriate category subdirectories. Compiler fixes: (1) FP array subscript write now emits `movss [rbx], xmm0` / `movsd [rbx], xmm0` instead of `mov [rbx], rax`; (2) FP array subscript read now emits `movss xmm0, [rax]` / `movsd xmm0, [rax]` instead of the truncating `mov rax, [rax]` + `movsxd rax, eax`; (3) `expr_result_type` for `AST_ARRAY_INDEX` now returns `TYPE_FLOAT`/`TYPE_DOUBLE` for FP element types instead of `TYPE_LONG`, so FP binary ops use the correct SSE2 path; (4) nested-brace local array initializers (`int A[2][3] = {{1,2,3},{4,5,6}}`) now populate all elements via a new `emit_local_array_init` recursive helper; (5) nested-brace global array initializers work via a new `emit_global_array_elements` helper; (6) mixed FP/int ternary branches (`a ? float_val : int_val`) now widen both branches to a common FP type before the merge point; (7) string literal subscript (`"abc"[2]`) is now supported in both the parser and codegen. Four test files were also corrected for Linux exit-code range (8-bit truncation). All 1841 tests pass (1161 valid, 256 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified with no source changes during bootstrap.

Through stage 112 (FP calling convention, va_arg for double, and math.h):

> Stage 112 completes floating-point support through the SysV AMD64 ABI calling convention. FP arguments (float/double) go in xmm0–xmm7 (counted independently from GP args in rdi–r9); non-variadic function prologues move xmmN values to local stack slots; variadic prologues save xmm0–xmm7 to the register-save area (176 bytes: 48 GP + 128 XMM); `al` is set to the XMM register count before variadic calls; `va_arg(ap, double)` fetches from reg_save_area or overflow_arg_area; `va_arg(ap, float)` is rejected per C99 (float is always promoted to double). A `test/include/math.h` stub declares common math functions so programs can call `sqrt`, `pow`, etc. from libm. All 1650 tests pass (965 valid, 255 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 verified.

Through stage 111 (float/double comparisons and boolean contexts):

> Stage 111 adds float/double values in comparison expressions, logical operators, and control-flow conditions. The six relational and equality operators (`<`, `<=`, `>`, `>=`, `==`, `!=`) on FP operands use SSE2 `ucomiss`/`ucomisd` with NaN-correct `set*` sequences: `==` requires `sete+setnp+and` (NaN==NaN is false per C99), `!=` requires `setne+setp+or` (NaN!=NaN is true). FP values in `if`/`while`/`for`/`do-while` conditions and ternary `?:` are converted to 0/1 in `rax` via a new `emit_fp_bool_to_rax` helper. Logical NOT (`!`) on FP uses `sete+setnp+and` so `!NaN==0`. Mixed FP/int comparisons promote the integer side via `cvtsi2ss`/`cvtsi2sd`. All 1643 tests pass (958 valid, 255 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 cycle passes cleanly with no source changes needed during bootstrap.

Through stage 108 (#elifdef / #elifndef):

> Stage 108 adds `#elifdef NAME` and `#elifndef NAME` branch-transition directives (C23 §6.10.1 / GCC/Clang extension), equivalent to `#elif defined(NAME)` and `#elif !defined(NAME)` respectively. The change is entirely confined to `src/preprocessor.c`; both new branches are placed before `#elif` in the directive chain so they take priority and correctly update `cond_stack` state even inside inactive regions. All 1621 tests pass (936 valid, 255 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 cycle passes cleanly.

Through stage 107 (inline keyword, assert.h, and va_copy codegen):

> Stage 107 closes three independent C99 gaps: the `inline` function specifier is now parsed (consumed and discarded via the same pattern as `restrict` and `volatile`), the `<assert.h>` stub header is available with a complete NDEBUG-aware `assert` macro, and `va_copy` codegen is corrected to emit three 8-byte moves copying the 24-byte SysV AMD64 `va_list` struct (previously a silent no-op). A preprocessor bug was discovered and fixed: `__FILE__` and `__LINE__` were not expanding inside function-like macro bodies during rescan; fixed via static globals tracking current file/line context. All 1615 tests pass (930 valid, 255 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 cycle passes cleanly.

Through stage 104 (complete constant-expression evaluators):

> Stage 104 extends both compile-time constant-expression evaluators (token-stream in parser.c for case labels, enum values, array designators, and file-scope initializers; AST-based in codegen.c for block-scope static scalars) to support the complete C99 integer constant expression operator set: relational operators (<, <=, >, >=), equality operators (==, !=), logical AND (&&), logical OR (||), and the ternary conditional operator (?:). Also fixes a pre-existing precedence bug where eval_const_additive and eval_const_shift had their call order inverted, making 3+1<<2 evaluate as 7 instead of 16. No new tokens, AST node types, or grammar rules. All 1584 tests pass (909 valid, 255 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 cycle passes cleanly.

Through stage 103 (block-scope static scalar constant-expression initializers):

> Stage 103 extends block-scope static scalar initializers to accept the full set of compile-time constant expressions — arithmetic, bitwise, shift, unary, and `sizeof(type-name)` — matching the expressiveness already supported for `case` labels, enum values, and file-scope globals. Previously only integer literals, character literals, and negated literals were accepted. The implementation adds a recursive `eval_const_init` helper in `src/codegen.c` that evaluates parsed AST subtrees; the helper mirrors the parser's `eval_const_expr` function but operates post-parsing on the code-generator AST. No parser, AST, or grammar changes. Two bugs discovered during testing: hex literals require `strtol` base 0 (auto-detect) rather than base 10, and scalar `sizeof` requires a fallback to `type_kind_bytes` when `full_type` is NULL. All 1569 tests pass (894 valid, 253 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 cycle passes cleanly with all 1569 tests.

Through stage 101 (block-scope static aggregates):

> Stage 101 enables block-scope `static` local variables to hold aggregate types (arrays, structs, and unions) in addition to scalars and pointers. The feature uses existing infrastructure from stage 71 (static-variable label generation and registration) and extends four codegen sites to handle RIP-relative addressing for aggregate statics. Initialized arrays and structs are populated from brace-list initializers; char arrays can also be initialized from string literals. All 1552 tests pass (880 valid, 250 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 cycle passes cleanly with no compiler source changes needed.

Through stage 100 (file-scope constant expressions):

> Stage 100 enables file-scope integer-typed variables to accept full compile-time constant expressions (arithmetic, bitwise, shift, and unary operators) as initializers, matching the expressiveness already used in `case` labels and enum enumerator values. A new `sizeof(type-name)` operator is added to the constant-expression evaluator to support patterns like `int BUF_SZ = sizeof(int) * 1024;`. Expressions are evaluated at parse time and stored as integer literals in the AST. The evaluator is wired into two sites in the file-scope declarator path: first declarators and multi-declarator lists. Pointer-typed and struct/union-typed globals retain literal-only initialization (null pointers, string literals, etc.). All 1544 tests pass (874 valid, 248 invalid, 86 integration, 50 print-ast, 100 print-tokens, 21 print-asm; 165 unit). Self-host C0→C1→C2 cycle passes cleanly.

Through stage 99 (typedef enum completion):

> Stage 99 completes the `typedef enum` feature with full support for integer constant-expression enumerator values and forward-declared enum tags. Enumerators now accept arbitrary constant expressions (arithmetic, bitwise, shift operators, parentheses, and references to previously-defined enum constants) instead of just literals. The pattern `typedef enum Status Status;` before the definition now works, enabling idiomatic C99 code. Internally, the three single-operator `eval_case_const_*` helpers were replaced with a unified nine-level constant-expression evaluator (primary → unary → multiplicative → shift → additive → bitwise-and/xor/or) matching C99 precedence rules. All 1531 tests pass (864 valid, 246 invalid, 86 integration, 49 print_ast, 100 print_tokens, 21 print_asm). Self-host C0→C1→C2 cycle passes cleanly.

Through stage 98 (compound literals):

> Stage 98 ships C99 compound literals — `(type-name){ initializer-list }` creates an unnamed temporary object on the stack that is a modifiable lvalue. Struct/union compound literals (e.g. `(struct Point){ .x = 1, .y = 2 }`), array compound literals with explicit or inferred size (e.g. `(int[]){ 1, 2, 3 }`), and scalar compound literals (e.g. `(int){ 7 }`) are all supported. Compound literals can be passed as function arguments, assigned to variables, used as postfix bases for `.field`/`[index]`, and have their address taken with `&`. A two-phase pre-scan (`scan_expr_compound_literals`) assigns each literal an rbp-relative stack slot before the prologue is emitted. Compound literals at file scope are diagnosed. All 1521 tests pass (855 valid, 246 invalid, 86 integration, 48 print_ast, 100 print_tokens, 21 print_asm). Self-host C0→C1→C2 cycle passes cleanly.

Through stage 97 (designated initializers):

> Stage 97 ships C99 designated initializers (`.member = value` for struct fields and `[index] = value` for array elements), enabling sparse and out-of-order field/element initialization with automatic zero-fill for skipped positions. The implementation adds `AST_DESIGNATED_INIT` node type, extends the parser to dispatch on designator syntax, and rewrites four codegen paths (local struct, local array, global struct, global array) with cursor or slots-mapping patterns. Chained designators (`.a.b`, `.arr[2]`) are detected and rejected. All 1501 tests pass (843 valid, 242 invalid, 85 integration, 45 print_ast, 100 print_tokens, 21 print_asm). Self-host C0→C1→C2 cycle passes cleanly.

Through stage 95-11 (remove static char arrays from codegen.h):

> Stage 95-11 converts the remaining `char[MAX_NAME_LEN]` name/label buffers in codegen.h structs to `const char *` pointers: `LocalVar.name`, `LocalVar.static_label`, `LocalStaticVar.label`, `GlobalVar.name`, and `GlobalVar.init_label`. Names from AST/lexer-owned storage assign the pointer directly; generated labels (`Lstatic_*` and `Lstr*`) use `util_strdup`. The 2D array `char user_labels[MAX_USER_LABELS][MAX_NAME_LEN]` in `CodeGen` (plus `user_label_count`) is replaced with `Vec user_labels; /* const char * */`, removing the 64-label-per-function cap entirely. `MAX_USER_LABELS` is removed from `include/constants.h`. The only remaining `MAX_NAME_LEN` application is `StructField.name` in `type.h`. All 1479 tests pass (165 unit, 831 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).

Through stage 95-10 (remove static char arrays from parser.h):

> Stage 95-10 converts seven `char field[MAX_NAME_LEN]` embedded arrays in parser.h structs to `const char *` pointers: `EnumConst.name`, `EnumTag.tag`, `StructTag.tag`, `UnionTag.tag`, `TypedefEntry.name`, `FuncSig.name`, and `GlobalObjSig.name`. All `strncpy` copy operations are replaced with direct pointer assignments. Three local `char[256]` temporary buffers (used during struct, union, and enum specifier parsing) are simplified to `const char *` pointers into the lexer string pool. The `MAX_NAME_LEN` limit is removed from all parser.h struct name/tag fields. All name and tag values derive from `parser->current.value` (a persistent pointer into lexer-owned storage since stage 95-08), making direct pointer assignment safe. All 1479 tests pass (165 unit, 831 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).

Through stage 95-09 (remove static char array from ASTNode and parser):

> Stage 95-09 changes `ASTNode.value` from `char value[MAX_NAME_LEN]` (a fixed 256-byte embedded buffer) to `const char *value` (a pointer into lexer-owned storage). All parser sites that previously memcpy'd token bytes into the node's value buffer are replaced with simple pointer assignments. String literal concatenation uses a StrBuf scratch buffer plus `lexer_store_bytes`. The `ParsedDeclarator.name` field is also changed to `const char *` to avoid dangling pointers when identifier names are stored in AST nodes. Two bug fixes: case label values and enum-constant integer literals were formatted into local `char[32]` buffers and passed directly to `parser_node`; these are now stored in the lexer pool via `lexer_store_bytes` first. The `MAX_NAME_LEN` limit on `ASTNode.value` is removed; string literals longer than 255 bytes are now handled correctly. All 1479 tests pass (165 unit, 831 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).

Through stage 95-07 (convert remaining static usages):

> Stage 95-07 converts two LOW-priority fixed-capacity items in the code generator. `MAX_SWITCH_DEPTH`: `CodeGen.switch_stack[16]` (a `SwitchCtx` array) is replaced with `Vec switch_stack; /* SwitchCtx */`, eliminating the hard nesting-depth cap on `switch` statements. `MAX_SWITCH_DEPTH` removed from `include/constants.h`. `MAX_CALL_LAYOUT_ITEMS`: a `compile_error` guard is added to `compute_call_layout`, fixing a previously unchecked silent overflow risk if a call had >24 arguments. No language features were added. All 1471 tests pass at C0, C1, and C2 (165 unit, 823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).

Through stage 95-06 (convert high-risk static usages):

> Stage 95-06 converts four HIGH-priority fixed-capacity static arrays in the parser and code generator to use the `Vec` dynamic growable-array module: `PARSER_MAX_STRUCT_FIELDS` (two local stack arrays in struct/union body parsing with a silent data-loss bug where fields beyond 64 were silently dropped due to hardcoded overflow check), `MAX_BREAK_DEPTH` (CodeGen.break_stack with no bounds check, silent buffer overflow risk), `PARSER_MAX_TYPEDEFS`, and `PARSER_MAX_FUNCTIONS`. All four had HIGH priority in the fixed-capacity inventory. Two latent bugs were fixed: the struct field overflow check used hardcoded `64` instead of the constant, and break-stack tracking had no bounds checking. All four corresponding constants were removed from `include/constants.h`. No language features were added. All 1471 tests pass at C0, C1, and C2 (165 unit, 823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).

Through stage 95-05 (convert medium-risk static usages):

> Stage 95-05 converts six medium-risk fixed-capacity static arrays in the parser and code generator to use the `Vec` dynamic growable-array module: `Parser.globals` (was `GlobalObjSig[256]`), `Parser.enum_consts` (was `EnumConst[256]`), `Parser.struct_tags` (was `StructTag[32]`), `CodeGen.locals` (was `LocalVar[256]`), `CodeGen.globals` (was `GlobalVar[256]`), and `CodeGen.string_pool` (was `ASTNode*[2048]`). All six had Safe Realloc = YES and MEDIUM priority in the fixed-capacity inventory; the six corresponding constants were removed from `include/constants.h`. Two latent bootstrap defects were found and fixed: the C0 parser mishandled `*(T**)expr` cast-of-dereference patterns (split into two statements), and `vec_push`'s capacity overflow check emitted signed division — rewritten using local `size_t` variables (same root cause as the `vec_reserve` fix in stage 95-04). No language features were added. All 1471 tests pass at C0, C1, and C2 (165 unit, 823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).

Through stage 95-04 (convert low-risk static usages):

> Stage 95-04 converts three lowest-risk fixed-capacity static arrays in the parser and code generator to use the `Vec` dynamic growable-array module: `Parser.enum_tags` (was `EnumTag[32]`), `Parser.union_tags` (was `UnionTag[32]`), and `CodeGen.local_statics` (was `LocalStaticVar[128]`). All three had Safe Realloc = YES and LOW priority in the fixed-capacity inventory. Two latent bootstrap defects were also found and fixed: `src/vec.c` and `src/strbuf.c` were missing from `build.sh`'s SRC_FILES, and `vec_reserve`'s overflow check emitted signed division (`idiv`) for struct-member operands — rewritten using local `size_t` copies to guarantee unsigned `div`. No language features were added. All 1471 tests pass at C0, C1, and C2 (165 unit, 823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).

Through stage 95-03 (dynamic string buffer module):

> Stage 95-03 adds a `StrBuf` dynamic character/string buffer module
> (`include/strbuf.h`, `src/strbuf.c`) modeled after the `Vec` module from
> stage 95-02. The API provides `strbuf_init`, `strbuf_free`, `strbuf_reserve`,
> `strbuf_append_char`, `strbuf_append_str`, `strbuf_append_n`, and
> `strbuf_null_terminate` with doubling growth, overflow checks, and fatal-error
> reporting on allocation failure. `strbuf_null_terminate` writes a null byte at
> `data[len]` without incrementing `len`, making `data` a valid C string while
> keeping `len` as the character count. A new `test/unit/test_strbuf.c` adds 59
> assertions; the unit runner now builds and aggregates both Vec and StrBuf
> binaries (165 unit assertions total). No language features were added.
> All 1471 tests pass at C0, C1, and C2
> (165 unit, 823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).

Through stage 95-02 (Vec generic growable-array foundation):

> Stage 95-02 adds a reusable `Vec` generic growable-array module (`include/vec.h`,
> `src/vec.c`) as a foundation for future stages that will replace fixed-capacity
> compiler tables with dynamic storage. The API provides `vec_init`, `vec_free`,
> `vec_reserve`, `vec_push`, `vec_get`, `vec_pop`, and `vec_clear` with doubling
> growth, overflow checks, and fatal-error reporting on allocation failure.
> A new `test/unit/` suite of 106 assertions exercises all operations.
> No language features were added. All 1412 tests pass at C0, C1, and C2
> (106 unit, 823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).

Through stage 94 (self-hosting validation and implement-stage skill test):

> Stage 94 validates the implement-stage skill's 4-phase workflow and confirms
> bootstrap stability by successfully compiling the compiler with itself (C0 → C1 → C2).
> All 1306 tests pass at every stage. One bug was found and fixed during self-hosting:
> `build.sh` was missing `-I test/include` when invoking the bootstrap compiler,
> preventing resolution of stub system headers needed to compile the compiler's own source.
> The compiler reached a fixed point — C1 and C2 are behavior-identical. Timeout guards
> from stage 93 (300s per file, 30s per test) were confirmed active during the bootstrap
> cycle. No new compiler language features were added; this is a process/validation stage.
> All 1306 tests pass (823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).

Through stage 92 (self-compilation validation):

> Stage 92 validates self-compilation of the ClaudeC99 compiler. The compiler
> (C0, built with gcc) successfully compiles itself to produce C1, which passes
> all 1306 tests. C1 then compiles itself to produce C2, which also passes all
> 1306 tests, confirming bootstrapping stability. Six silent code generation
> bugs were discovered and fixed during self-compilation, all related to
> struct-by-value assignments and sizeof calculations. The compiler is now
> fully self-hosting.

Through stage 91 (address-of member lvalues):

- **Preprocessor**:
  - _Comments and line splicing_: comment removal (`//` and `/* */`) with
    space replacement; line splicing (backslash-newline continuations).
  - _File inclusion_: `#include "file.h"` local inclusion, searched relative
    to the including file's directory; nested includes supported; recursive
    includes detected via a depth limit. `#include_next <file.h>` (GCC
    extension) searches for the named file starting from the `-I` directory
    *after* the one that contains the current file, enabling GCC wrapper
    headers (e.g. `gcc/include/stdint.h`) to forward to the real system header.
  - _Stub system headers_: controlled stubs for `stdio.h` (with opaque `typedef struct FILE FILE` pointer type, `#define EOF (-1)`, standard stream pointers `stdin`, `stdout`, `stderr`, and declarations for `fopen`, `fclose`, `fgetc`, `fgets`, `fprintf`, `snprintf`, `vfprintf`, `vprintf`, `vsnprintf`, `putchar`, `fseek`, `ftell`, and `fread`; and file-position/read macros `SEEK_SET`, `SEEK_CUR`, `SEEK_END`), `stddef.h`,
    `stdlib.h` (with `malloc`, `realloc`, `calloc`, `free`, `exit`, `strtol`, `strtoul`), `string.h` (with `strcmp`, `strlen`, `memcpy`, `memset`, `memcmp`, `strchr`, `strncpy`, `strncat`, `strncmp`, `strcpy`, `strrchr`), `limits.h` (with `UINT_MAX` and `ULONG_MAX`),
    `stdint.h`, `stdbool.h`, `ctype.h` (character classification and conversion),
    `errno.h` (error constants and `errno` macro), `time.h` (`time_t`, `clock_t`,
    `time()`, `clock()`), `setjmp.h` (`jmp_buf`, `setjmp`, `longjmp`), and `stdarg.h` (`va_list` typedef and va_* macro stubs),
    supplied from `test/include/`.
  - _Object-like macros_: `#define` definition and expansion. Macros defined
    in headers are visible to the including file; compatible redefinitions are
    allowed, incompatible ones rejected.
  - _Function-like macros_: `#define` with argument substitution, nested
    invocations, and exact argument-count checking.
    - Stringification: `#param` converts the argument tokens to a C string
      literal (whitespace normalized to a single space, `"` and `\` escaped;
      the argument is not expanded first). `#` in object-like macros and `#`
      not followed by a parameter name are rejected at `#define` time.
    - Token pasting: `##` concatenates adjacent tokens (surrounding whitespace
      discarded). `##` at the start/end of a replacement list and `##` in
      object-like macros are rejected at `#define` time.
    - Variadic forms: `#define M(...)` and `#define M(x, ...)`, with
      `__VA_ARGS__` substituting the extra arguments as a comma-separated
      token sequence.
  - _Directive recognition_: unsupported directives are rejected with a
    diagnostic error.
  - _Conditional compilation_: `#ifdef`/`#ifndef`/`#else`/`#endif`, plus
    `#if`/`#elif` (below), plus `#elifdef`/`#elifndef` (C23/GCC/Clang
    extension equivalents of `#elif defined(NAME)` / `#elif !defined(NAME)`),
    with first-true-wins branch semantics. Inactive regions are fully skipped
    — not emitted, not macro-expanded, and nested `#define`/`#include`
    suppressed. Nesting up to 64 levels deep. Errors on a missing `#endif`,
    unmatched `#else`/`#endif`, duplicate `#else`, `#elif` without a
    conditional, and `#elif` after `#else`.
  - _`#if`/`#elif` expression operands_: integer literals (including hexadecimal form `0x`/`0X`); `defined(NAME)` and
    `defined NAME`; object-like macros that expand to integer literals
    (`0` = false, nonzero = true); function-like macro calls (pre-expanded per C99 §6.10.1); bare undefined identifiers (evaluate to 0);
    parenthesized expressions with arbitrary nesting; and macros expanding to
    negative literals (e.g. `#define VALUE -1`).
  - _`#if`/`#elif` operators_, loosest to tightest precedence: ternary `?:` (lowest, right-associative); logical `||`
    then `&&`; bitwise `|`, `^`, `&`; equality `==`, `!=`; relational `<`,
    `<=`, `>`, `>=`; shift `<<`, `>>`; additive `+`, `-`; multiplicative `*`,
    `/`, `%`; unary `!`, `-`, `+`, `~` (chainable, e.g. `!-1`). Division or
    modulo by zero is a fatal error.
  - _`#undef`_: `#undef NAME` removes a macro from the table (a no-op if the
    name is undefined).
  - _`#error`_: `#error message` halts compilation with the message text;
    silently skipped inside inactive conditional regions.
  - _Predefined macros_: `__FILE__` (current source file), `__LINE__` (current
    line), `__DATE__` (`"Mmm DD YYYY"`), `__TIME__` (`"HH:MM:SS"`),
    `__STDC_HOSTED__` (`1`). Standard macros `__STDC__` (`1`),
    `__STDC_VERSION__` (`199901`), and `__CLAUDEC99__` (`1`) behave like
    ordinary object-like macros (can be `#undef`'d; incompatible redefinitions
    rejected). Static target/ABI macros reflecting the x86_64 Linux LP64 ABI —
    `__x86_64__`, `__linux__`, `__unix__`, `__LP64__`, `_LP64`,
    `__CHAR_BIT__`, and the `__SIZEOF_*__` family (`CHAR`, `SHORT`, `INT`,
    `LONG`, `POINTER`, `SIZE_T`, `LONG_LONG`, `WCHAR_T`) — are injected
    unconditionally and defined in the preprocessor code rather than a header.
    Wide-character ABI macros `__WCHAR_MAX__` (`0x7fffffff`),
    `__WCHAR_MIN__` (`(-__WCHAR_MAX__ - 1)`), and `__WCHAR_TYPE__` (`int`)
    are also injected, matching GCC's built-in definitions for x86_64 Linux.
  - _Command-line options_: `-DNAME` (defines `NAME` as `1`) and
    `-DNAME=VALUE`, injected before preprocessing. Include search paths via
    `-I<dir>` or `-I <dir>` (repeatable): quoted includes search the including
    file's directory first, then `-I` directories in order; angle-bracket
    includes search `-I` directories only, in order.
- **Statements**: `if/else`, `while`, `do/while`, `for` (with C99 declaration initializers), `switch/case/default`
  (case labels support integer literals, character literals, enum constants, and compile-time constant expressions
  with unary/binary `+`/`-` operators; discriminant may be any integer type including `long`, `long long`, and
  `unsigned long long`), `break`, `continue`, `goto`/labels, block scopes with shadowing.
- **Declarations**: comma-separated init-declarator lists (e.g., `int a, b;`,
  `int a=3, b=4;`, `int *p, q;`). Parenthesized declarators provide grouping syntax
  for pointers (e.g., `int (*p)` and `int (**pp)`) with semantics equivalent to
  unparenthesized forms. Typedef aliases for integer scalar types, pointer types
  (e.g., `typedef int *IntPtr;`, `typedef char *String;`), array types (e.g., `typedef int A[4];`),
  function pointer types (e.g., `typedef int (*BinaryFn)(int, int);`), complete struct types
  (e.g., `typedef struct Point { int x; int y; } Point;`), and incomplete struct forward
  declarations (e.g., `typedef struct ASTNode ASTNode;` before the body is defined) with full
  type chain support, block-scope tracking, and shadowing. The typedef name can be used as a type specifier in variable
  declarations, assignments, multi-declarator lists, and (for function pointers) indirect calls.
  Enum declarations (named and anonymous) with auto-incrementing or explicit integer constant expression values
  (arithmetic, bitwise, shift operators, parentheses, and references to previously-defined enum constants);
  enum constants are available as compile-time integer values throughout the translation unit.
  Forward-declared enum tags (`typedef enum Status Status;` before the body) are supported.
  Block-scope `static` variables (scalar, pointer, array, and struct/union types) persist values across function calls and are stored in .bss or .data with constant-only initializers.
  `auto` storage-class specifier is accepted at block scope (equivalent to default automatic storage).
  `register` storage-class specifier is accepted at block scope and as a function parameter qualifier (allocates like automatic; address-of on a register variable is a compile error).
- **Integer types**: `char`, `short`, `int`, `long` and their `unsigned` variants
  (`unsigned char`, `unsigned short`, `unsigned int`, `unsigned long`, plain `unsigned`).
  `signed` keyword support (`signed char`, `signed short`, `signed int`, `signed long`,
  plain `signed` → int). Trailing `int` for short and long forms (`short int`, `long int`,
  `unsigned short int`, `unsigned long int`, `signed short int`, `signed long int`).
  Usual arithmetic conversions (UAC) handle mixed signed/unsigned arithmetic. Unsigned
  loads use zero-extension (`movzx`); unsigned division uses `div`; unsigned right shift
  uses `shr` (logical); unsigned comparisons use unsigned condition codes. Integer
  literals with suffixes: `L` (long), `U` (unsigned), `UL`/`LU` (unsigned long),
  `LL`/`ll` (long long), `ULL`/`LLU` (unsigned long long).
  Hexadecimal integer literals (`0x`/`0X` prefix, e.g. `0x2A`, `0xffffffff`, `0x10ULL`)
  with the same suffix forms.
  `long long` and `unsigned long long` types (8 bytes, alignment 8); codegen identical to `long`.
  Predefined macro `__SIZEOF_LONG_LONG__` (= `8`).
  _Bool type with value-normalization semantics (any nonzero value assigned to _Bool becomes 1; zero stays 0); integer promotion applies in expressions.
- **Functions**: multiple functions per translation unit, forward
  declarations with compatibility checking (return type and parameter type
  matching between declarations and definitions), distinction between `int f();` (no prototype information)
  and `int f(void);` (zero-parameter prototype) per C99 §6.7.5.3p14, SysV AMD64 calls with any number of integer, pointer, _Bool, enum, and struct-pointer arguments (args 1–6 in registers per ABI, args 7+ passed on the stack right-to-left with automatic 16-byte alignment), typed parameter and return-type conversions at the call boundary,
  struct/union arguments and return values passed **by value** per the SysV AMD64 ABI (register-class objects ≤ 16 bytes use 1–2 INTEGER GP registers for arguments and rax:rdx for returns; memory-class objects > 16 bytes are passed on the stack and returned through a hidden pointer / sret in rdi; the callee receives a private copy and the caller's original is unchanged),
  calls into libc via `extern` emission for declared-but-not-defined functions
  with support for void* parameters/returns (e.g., `malloc`, `free`), const char*
  parameters (e.g., `puts`, `strcmp`, `strlen`), and typedef-based size_t.
  `static` functions have internal linkage (no `global` NASM directive emitted).
  Command-line argument support: `int main(int argc, char **argv)` signature with
  argc and argv[i] access for string arguments passed at program invocation.
  Default argument promotions for no-prototype calls: functions declared without a prototype apply C99 default argument promotions to all arguments (narrow integer types char/short/unsigned short promote to int; float promotes to double).
  Variadic function declarations and definitions (e.g., `int f(int x, ...)`) with caller compatibility checking (actual args >= fixed params); callee-side access to extra arguments via `va_list`, `va_start`, `va_end`, `<stdarg.h>`: variadic function prologues save all 6 GP argument registers (rdi–r9) and all 8 XMM argument registers (xmm0–xmm7) to a 176-byte register-save area (48 GP + 128 XMM, 16-byte aligned); `__builtin_va_start` initializes all four `va_list` fields (gp_offset, fp_offset, overflow_arg_area, reg_save_area) per the SysV AMD64 ABI; `__builtin_va_end` is a no-op; `va_arg` extraction for GP types (int, unsigned int, long, unsigned long, long long, unsigned long long, pointer) and `double` (via fp_offset into the XMM save area); `va_copy` copies the 24-byte `va_list` struct via three 8-byte moves; `va_arg(ap, float)` is rejected (float is promoted to double in variadic calls per C99 §6.5.2.2p6). Code generation emits `mov al, <xmm_count>` before variadic calls to set the SSE register count per the SysV AMD64 ABI. Callee-saved `rbx` is preserved in every function prologue and epilogue per the SysV AMD64 ABI.
- **Pointers**: pointer types, `&` and `*` as rvalue and lvalue,
  assignment through pointer, pointer parameters and return types,
  `NULL` as a null pointer constant. The address-of operator `&` accepts
  any addressable lvalue: identifiers, array subscripts, struct/union
  member access via `.` (e.g. `&s.x`) and `->` (e.g. `&p->x`).
  Pointer relational comparisons (`<`, `<=`, `>`, `>=`) are supported
  in addition to equality comparisons (`==`, `!=`).
  Equality comparisons accept non-zero integer constant operands as a
  GCC/Clang extension (e.g., `p == 1`); null pointer constant (`p == 0`)
  and pointer-to-pointer equality continue to work per C99.
  Equality comparisons between `void *` and any typed object pointer are
  accepted per C99 §6.5.9 (e.g., `fp == NULL` when `NULL` is `((void *)0)`).
- **void type**: `void` return type for functions; void functions may use bare `return;`
  or fall off the end without an explicit return. `void *` generic object pointer with
  implicit conversion to/from any non-function pointer type. `f(void)` parameter list
  means zero parameters.
- **const qualifier**: `const` keyword support in declarations and parameters; assignment
  to const-qualified variables is rejected. Pointer-level const enforcement: writes through
  const pointers (e.g., `*p = v` where `const int *p`) are always rejected; const-to-non-const
  conversions in pointer assignments (e.g., `int *q = p` where `const int *p`) issue a warning
  (or error if `-Werror` is set); const-to-pointer conversions (e.g., `int *q = &x` where
  `const int x`) are allowed as rvalue decay with const-discard warning/error semantics.
  Struct and union member declarations support `const` qualifiers: `const char *name` stores a pointer-to-const member; `const int kind` stores a const scalar member whose direct assignment is rejected. Array subscript writes through const-pointer members (e.g., `s.p[0] = 'H'` where `s.p: const char *`) are properly rejected at compile time. Assignments to any member of a `const`-qualified struct/union object (e.g., `const struct S s; s.x = 1`) and assignments through a pointer to const struct (e.g., `const struct S *p; p->x = 1`) are rejected.
- **volatile qualifier**: `volatile` keyword is recognized as a type qualifier in declarations,
  parameters, and type names. Volatile qualifiers are parsed and stored in the type, but have
  no code generation or semantic enforcement effects yet (writes to volatile fields are permitted).
- **Function pointers**: declarations of function-pointer typed variables (local, file-scope,
  static, extern) and parameters with full type compatibility checking across redeclarations.
  Function-pointer types are distinguished by return type, parameter count, and parameter types.
  Unnamed parameters are supported in function prototypes and function pointer signatures.
  Assignment and initialization of function pointers from function designators (function names),
  with strict type compatibility validation (return type and parameter types must match).
  Indirect calls through function pointer variables—both `fp(args)` and `(*fp)(args)`
  (explicit dereference) forms—with full argument-count and argument-type validation.
  Function pointer parameters work correctly, and function pointers can be passed as arguments
  to other functions. **Functions returning function pointers** (`int (*f())(int)`) are now supported (stage 137).
- **Arrays**: array declarations, indexing, array-to-pointer decay
  (including array-typed struct/union members, which decay to a pointer to
  their first element in a value context — e.g. when passed to a pointer parameter),
  pointer arithmetic (p + n, p - n, p++, p--, p += n, p -= n, p1 - p2 with element-size scaling),
  subscript-as-pointer-arithmetic, nested subscripting of arrays of pointers (e.g., `names[0][1]`).
  Multidimensional array declarations and indexing (`int A[M][N]`, `A[i][j]`), multidimensional
  array members in structs and unions, correct type nesting (right-to-left, e.g., `int A[4][8]`
  is array[4] of array[8] of int), array-to-pointer decay at each subscript level, and `sizeof`
  support for multidimensional arrays in both type-name form (`sizeof(int[4][8])`) and expression
  form (`sizeof(s.table[0])` where s.table is multidimensional). Maximum 8 array dimensions supported.
  Initialization of local `char` arrays from a string literal (with explicit or
  inferred size), brace-enclosed initializer lists for local arrays with
  partial initialization and automatic zero-fill (e.g., `int a[3] = {1, 2, 3};`),
  file-scope (global) uninitialized array declarations. File-scope array initialization from
  string literals (`char s[] = "abc"`) and brace-enclosed constant lists (`int values[] = {10,20,30}`,
  `char *names[] = {"if","else","while"}`). Size inference from initializer for `[]` declarations
  at both block scope and file scope. Arrays of pointers with strict
  type checking: compatible pointer assignments allowed, void-pointer conversions bidirectional,
  null constant (0) accepted, nonzero integers and incompatible pointer types rejected.
  Pointer arithmetic rejects void* and function pointer operands.
- **Structs and Unions**: named struct and union definitions with natural-alignment field layout,
  local and global struct/union variables, sizeof operator on struct/union types and instances,
  member access via "." (dot) and "->" (arrow) operators in both rvalue and lvalue contexts,
  chained arrow access (e.g., `n.next->value`),
  array-typed struct/union fields with subscript chaining (e.g., `p->tokens[i].kind`),
  brace-enclosed initializer lists for local struct/union variables with automatic zero-fill
  (e.g., `struct Point p = {3, 4};`), including char-array members initialized from a string
  literal in a brace initializer (e.g., `struct S s = {"Hello"};` where `s.name` is `char[32]`),
  whole-struct/union copy assignment and copy initialization
  from another variable of the same type. Pointer-to-struct/union function parameters
  (`struct T *p`) with `->` field access and mutation; `&local_struct` and `&global_struct`
  pass correctly as arguments; `(*p).field` deref-dot syntax works. Struct/union types as members
  of other structs (nested structs) with chained member access (e.g., `r.origin.x`).
  Arrays with struct/union element types (e.g., `struct Point points[10]`) with indexed member
  access patterns (e.g., `points[0].x`). Incomplete struct/union forward declarations:
  `struct Tag;` can appear before the body is defined; pointer-to-incomplete-struct/union fields
  and self-referential types via typedef are supported.
  Validation: `sizeof` and variable declarations reject incomplete struct/union types with diagnostic errors.
  File-scope struct/union initialization from brace-enclosed lists (`struct Point p = {3, 4};`).
  File-scope arrays of structs with per-element brace lists (`struct Point pts[] = {{1,2},{10,20}};`).
  Struct/union fields of type `char *` initialized from string literals in file-scope contexts.
  Field-level type checking for aggregate initializers: string literal for non-pointer field
  and non-null integer for pointer field are rejected.
  Bit-field members: integer fields with `:width` declarators (both named and anonymous forms),
  GCC-compatible storage-unit packing, and read-modify-write assignment semantics.
  Flexible array members: unsized arrays as the last member of a struct (with at least one
  prior named member), excluded from sizeof, supporting indexed access through pointers.
  Named `union` types with max-size layout (size = max member, all offsets 0), local and global
  union variables, first-member initialization, whole-union assignment, union fields inside structs,
  and struct fields inside unions.
  Anonymous struct and union types (without a tag) when a body is present
  (e.g., `struct { int x; int y; } p;`, `typedef union { int a; char b; } U;`).
  Each anonymous definition creates a unique type; two separately defined anonymous
  structs with identical layouts are not assignment-compatible.
- **File-scope objects**: file-scope (global) object declarations (scalars,
  pointers, arrays, structs), both initialized (integer-typed scalars accept full compile-time
  constant expressions with arithmetic, bitwise, shift, and unary operators, plus `sizeof(type-name)`;
  string-literal initialization for pointer globals; address-constant initialization for pointer
  globals and block-scope static pointers (`&global`, `&global[N]`, and function designators per C99 §6.6p9);
  brace-list initialization for array globals;
  and aggregate-initializer support for struct globals and arrays of structs,
  emitted to `.data` and `.rodata`) and uninitialized (with zero-initialization, emitted to
  `.bss`). Duplicate declarations and type mismatches are rejected. Function
  and object names do not collide in the ordinary identifier namespace.
  `extern` and `static` storage class specifiers are supported: `extern` declares
  without allocating storage; `static` declares with internal linkage.
- **String literals**: tokenization, AST node, static-data emission,
  decay to `char *` in expressions, decoded escape sequences (`\n`,
  `\t`, `\r`, `\\`, `\"`, `\0`), hex escapes (`\xNN`), and octal
  escapes (`\NNN`, 1–3 octal digits, e.g. `\0`, `\101`). Adjacent
  string literal tokens are automatically concatenated into a single
  literal (e.g., `"hello " "world"` becomes `"hello world"`).
- **Character literals**: tokenization, AST node, and codegen for
  `'A'`, `'0'`, the full simple-escape set (`\a`, `\b`, `\f`,
  `\n`, `\r`, `\t`, `\v`, `\\`, `\'`, `\"`, `\?`, `\0`), hex escapes
  (`\xNN`, e.g. `'\x41'` = 65), and octal escapes (`\NNN`, 1–3 octal
  digits, e.g. `'\101'` = 65). A character constant evaluates as an
  `int` (`'A'` = 65, `'\n'` = 10, etc.) and is a valid primary
  expression in any integer context: returns, initializers
  (`char`/`int`/`long`), assignment, arithmetic, comparison, `if`
  conditions, and as the right-hand side of an array element
  assignment. Malformed character literals (empty `''`,
  multi-character `'ab'`, unknown escapes such as `'\q'`, unterminated
  literals, and raw newline inside a literal) are rejected with
  diagnostic messages.
- **Compound assignment operators**: `+=`, `-=`, `*=`, `/=`, `%=`,
  `<<=`, `>>=`, `&=`, `^=`, `|=` on any modifiable lvalue (struct/union member
  access via `.` or `->`, array subscripts, pointer dereferences, and chains thereof).
  Each desugars to `lhs op= rhs` → `lhs = lhs op rhs`, enabling expressions like
  `p->cap *= 2`, `arr[i] += 2`, and `*p *= 2`, with all arithmetic, shift,
  and bitwise rules applying to the right-hand expression.
- **Arithmetic operators**: `+`, `-`, `*`, `/`, and `%` (remainder)
  on integer operands, with the usual `*`/`/`/`%` over `+`/`-`
  precedence and left-to-right associativity. `%` is integer-only:
  pointer or array operands are rejected.
- **Shift operators**: `<<` (left shift) and `>>` (arithmetic right
  shift) on integer operands. The result type follows the promoted
  type of the left operand (`char`/`short`/`int` → `int`; `long` →
  `long`); the right operand acts purely as a shift count. Pointer
  or array operands on either side are rejected. Shift binds less
  tightly than `+`/`-` and more tightly than `<`/`>`/`<=`/`>=`,
  matching C's standard precedence.
- **Bitwise binary operators**: `&` (bitwise AND), `^` (bitwise XOR),
  and `|` (bitwise OR) on integer operands. Operands undergo the
  usual integer promotions and the result follows the common
  arithmetic type (`char`/`short`/`int` → `int`; either side `long`
  → `long`). Pointer or array operands on either side are rejected.
  Precedence sits between equality and `&&`, ordered (tightest
  first) `&` > `^` > `|`, so `1 | 2 & 4` parses as `1 | (2 & 4)` and
  `1 & 3 == 3` parses as `1 & (3 == 3)`.
- **Unary operators**: `+`, `-`, `!` (logical not), `~` (bitwise
  complement), `++`/`--` (prefix and postfix on any modifiable lvalue including bare identifier, struct/union member via . or ->, array subscript, pointer dereference, and chains thereof; postfix yields the old value and prefix the new value; pointer ++/-- keeps element-size scaling; const-qualified and non-lvalue operands are rejected), `*` (dereference),
  `&` (address-of). `~` is integer-only: pointer and array
  operands are rejected. `~` follows the usual integer promotions
  (`char`/`short`/`int` → `int`; `long` → `long`).
- **`sizeof`**: `sizeof(<type>)` and `sizeof(<expression>)` return
  the byte size of the operand's type as a `size_t` (unsigned integer) value per C99.
  The non-parenthesized form `sizeof expr` is also supported. Supported
  scalar types: `char` (1), `short` (2), `int` (4), `long` (8), and
  any pointer type (8). Struct types return the total byte size based on
  natural-alignment field layout. For expression operands, integer promotions and
  usual arithmetic conversions apply to determine the result type
  (`sizeof(char_var + 1)` == 4 because char promotes to int). Pointer-arithmetic
  binary expressions (`ptr + int`, `arr + int`, `ptr - ptr`, `string_lit + int`) correctly
  return 8 on LP64 (pointer/ptrdiff_t size). The expression operand is never evaluated —
  side effects such as `x++`, `x = 5`, or function calls inside `sizeof` are suppressed.
  `sizeof(A)` where `A` is a declared array returns the total byte size
  (`element_size × element_count`) as a compile-time constant — no runtime
  code is emitted for the array operand and the array is not decayed to a
  pointer. The array-type-name form is also supported, including
  multidimensional shapes (e.g. `sizeof(int[10])` == 40,
  `sizeof(int[4][8])` == 128).
  `const`-qualified type names in `sizeof` are supported (e.g.,
  `sizeof(const int)`, `sizeof(const char *)`); the qualifier does not
  affect the computed size.
- **Conditional operator**: `condition ? expr_true : expr_false`. The condition is evaluated first; only the selected branch (true or false) is then evaluated and its value returned. The condition may be any integer or pointer expression. Both branches may be integer expressions (result is common type) or compatible pointer types (result is that pointer type). One branch may be a pointer with the other being the null constant `0`. The conditional expression is right-associative with lower precedence than logical OR and higher precedence than assignment.
- **Comma operator**: `expr1, expr2` evaluates both expressions left to right, discards the left result, and returns the right result. Comma is the lowest-precedence operator (below assignment), left-associative, and produces an rvalue. Comma-as-separator in function calls and initializers is preserved via parser-level precedence.

## Not yet supported

Anonymous struct/union members (C11 feature); block-scope `extern`;
compound literals at file scope; pointer-to-function-pointer;
object-file (`.o`) emission and separate linking (multi-file source compilation is now supported in a single invocation).

The authoritative grammar for the supported language is in
[`docs/grammar.md`](docs/grammar.md). The full per-feature checklist is in
[`docs/outlines/checklist.md`](docs/outlines/checklist.md).

## Tests

The test harness consists of seven suites under `test/`:

| Suite          | What it checks                                                      |
| -------------- | ------------------------------------------------------------------- |
| `unit`         | Standalone C unit tests for compiler utility modules (compiled with system GCC, not `build/ccompiler`). Each runner compiles and runs the test binary directly and outputs "Results: P passed, F failed, T total". |
| `valid`        | Compile, assemble, link, run; exit code must match `__N` in filename. If a sibling `<name>.expected` file is present, the program's stdout must also match it byte-for-byte. |
| `invalid`      | Compiler must reject the program                                    |
| `integration`  | Multi-file tests in subdirectories; compile all `.c` files, assemble, link against libc with `cc -no-pie`, run; companion files (`.expected`, `.libs`, `.cflags`, `.args`, `.input`, `.status`) drive expected stdout, link flags, compiler flags, runtime argv, stdin, and exit code. The runner always appends `-I${PROJECT_DIR}/test/include` after the test's own `.cflags` so stub system headers (`stdio.h`, `stddef.h`, etc.) are found automatically — `.cflags` files only need to list test-local include paths (e.g. `-Iinclude`). |
| `print_ast`    | `--print-ast` output must match `.expected`                         |
| `print_tokens` | `--print-tokens` output must match `.expected`                      |
| `print_asm`    | Generated `.asm` must match `.expected`                             |

Run everything from the project root after building:

```
./test/run_all_tests.sh
```

The runner aggregates per-suite results and prints a `Portable: P passed, F failed, T total` line. On Linux x86_64 it also runs `test/integration/run_tests_sysinclude.sh` (system-include suite) and `test/integration_sysinclude/run_tests.sh` (optional-library suite). As of stage 162 all 2065 portable tests pass (1286 valid, 261 invalid, 182 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit); the system-include suite passes 182/182; the optional-library suite passes 2/2 (test_sdl2_init, test_zlib_compress).

Individual suites can be run directly, e.g. `./test/valid/run_tests.sh`.

## Optional library tests

Integration tests under `test/integration_sysinclude/` may require optional
system libraries.  Tests for missing libraries are automatically skipped
(reported as `SKIP`, not `FAIL`).  The `.require` companion file in each test
directory names the prerequisite check command.

| Library | Debian/Ubuntu install     | Prerequisite check         | Test(s)                |
|---------|---------------------------|----------------------------|------------------------|
| SDL2    | `apt install libsdl2-dev` | `sdl2-config`              | `test_sdl2_init`       |
| zlib    | `apt install zlib1g-dev`  | `pkg-config --exists zlib` | `test_zlib_compress`   |

Future stages that add tests for other libraries (OpenGL, etc.) append rows to this table.

Tests in `test/valid/` use the naming convention
`test_<description>__<expected_exit_code>.c` so the runner can extract
the expected exit code from the filename.

### Test suite structure (stage 113)

The four large suites are organized into category subfolders. Runners
use `find`-based recursive discovery so new tests placed in any subfolder
are picked up automatically. Companion files (`.expected`, `.libs`) must
be placed in the **same subfolder** as their `.c` file.

#### `test/valid/` — 21 category subfolders

| Subfolder | Contents |
|-----------|----------|
| `arithmetic/` | add, subtract, multiply, divide, remainder, increment, decrement, negation |
| `bitwise/` | bitwise AND/OR/XOR/complement, shifts |
| `assignment/` | simple assignment, compound assignment operators, chained assignment |
| `comparisons/` | relational, equality, logical AND/OR, logical NOT |
| `casting/` | explicit casts, implicit conversions, usual arithmetic conversions |
| `control_flow/` | if/else, for, while, do-while, switch, break, continue, goto, return |
| `functions/` | function definitions, calls, prototypes, indirect calls, variadic functions |
| `pointers/` | pointer declarations, dereference, address-of, pointer arithmetic, void pointer |
| `arrays/` | array declarations, indexing, multidimensional arrays, array decay |
| `strings/` | string literals, adjacent concatenation |
| `chars/` | character literals, escape sequences |
| `structs/` | struct definitions, member access, nested structs, incomplete types |
| `unions/` | union definitions, member access |
| `enums/` | enum declarations and constants |
| `typedefs/` | typedef declarations and usage |
| `declarations/` | variable declarations, storage classes, integer type specifiers, `_Bool` |
| `expressions/` | compound literals, designated initializers, sizeof, comma, ternary |
| `preprocessor/` | macros, `#include`, `#if`/`#ifdef`, `#define`/`#undef`, predefined macros |
| `stdlib/` | printf, putchar, puts, stdlib.h, ctype.h, errno.h, setjmp.h, time.h |
| `floating_point/` | float and double arithmetic, comparisons, FP calling convention |
| `varargs/` | va_list, va_start, va_arg, va_end, va_copy |

New `test/valid/` tests go in the matching category subfolder. Tests that
do not fit any category go in `misc/`.

#### `test/invalid/` — `legacy/` + 9 category subfolders

| Subfolder | Contents |
|-----------|----------|
| `legacy/` | Numbered `test_invalid_NNN__*.c` tests |
| `aggregates/` | Struct and union type errors |
| `declarations/` | Storage-class and type-specifier errors |
| `types/` | `_Bool`, `void` type errors |
| `const/` | `const` assignment and write-through errors |
| `pointers/` | Pointer, arrow, dot, and subscript type errors |
| `functions/` | Variadic, function-pointer, and void-function errors |
| `expressions/` | Compound literal, conditional, designated-init, enum, switch-case errors |
| `control_flow/` | `for` declaration scope errors |
| `preprocessor/` | Preprocessor directive errors |

New `test/invalid/` tests go in the appropriate category subfolder
(or `legacy/` for backward-compatible numbered tests).

#### `test/print_ast/` — `legacy/` + `constructs/`

| Subfolder | Contents |
|-----------|----------|
| `legacy/` | `test_stage_NN_*` numbered regression tests |
| `constructs/` | Descriptively-named tests covering all language constructs |

New `test/print_ast/` tests go in `constructs/`.

#### `test/print_tokens/` — `tokens/` + `programs/`

| Subfolder | Contents |
|-----------|----------|
| `tokens/` | `test_token_*` tests (one per token type) |
| `programs/` | `test_program_*` tests (whole-program token streams) |

New `test/print_tokens/` tests go in `tokens/` or `programs/` as appropriate.

#### `test/print_asm/` — flat

All 21 tests follow `test_stage_*` naming; this suite stays flat.

## Development model

The project is built incrementally, one vertical stage at a time. Each
stage has:

- a spec in `docs/stages/`,
- an implementation summary in `docs/milestones/` once the stage is
  complete,
- new tests added to the relevant suites, and
- a clean run of the full test harness before the stage is closed.

## Requirements

- A C99 toolchain and CMake (>= 3.10) to build the compiler.
- `nasm` and `ld` to assemble and link the generated `.asm` files.
- Linux x86_64 to run the produced binaries.
