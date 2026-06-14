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
ccompiler [--version] [--print-ast | --print-tokens] [-Werror] [--max-errors=N] [--sysroot=<dir>] [-DNAME[=VAL]] [-I<dir>] <source.c> [<source2.c> ...]
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
    includes detected via a depth limit.
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
  - _`#if`/`#elif` expression operands_: integer literals; `defined(NAME)` and
    `defined NAME`; object-like macros that expand to integer literals
    (`0` = false, nonzero = true); bare undefined identifiers (evaluate to 0);
    parenthesized expressions with arbitrary nesting; and macros expanding to
    negative literals (e.g. `#define VALUE -1`).
  - _`#if`/`#elif` operators_, loosest to tightest precedence: logical `||`
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
    `LONG`, `POINTER`, `SIZE_T`) — are injected unconditionally and defined in
    the preprocessor code rather than a header.
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
  matching between declarations and definitions), SysV AMD64 calls with any number of integer, pointer, _Bool, enum, and struct-pointer arguments (args 1–6 in registers per ABI, args 7+ passed on the stack right-to-left with automatic 16-byte alignment), typed parameter and return-type conversions at the call boundary,
  struct/union arguments and return values passed **by value** per the SysV AMD64 ABI (register-class objects ≤ 16 bytes use 1–2 INTEGER GP registers for arguments and rax:rdx for returns; memory-class objects > 16 bytes are passed on the stack and returned through a hidden pointer / sret in rdi; the callee receives a private copy and the caller's original is unchanged),
  calls into libc via `extern` emission for declared-but-not-defined functions
  with support for void* parameters/returns (e.g., `malloc`, `free`), const char*
  parameters (e.g., `puts`, `strcmp`, `strlen`), and typedef-based size_t.
  `static` functions have internal linkage (no `global` NASM directive emitted).
  Command-line argument support: `int main(int argc, char **argv)` signature with
  argc and argv[i] access for string arguments passed at program invocation.
  Variadic function declarations and definitions (e.g., `int f(int x, ...)`) with caller compatibility checking (actual args >= fixed params); callee-side access to extra arguments via `va_list`, `va_start`, `va_end`, `<stdarg.h>`: variadic function prologues save all 6 GP argument registers (rdi–r9) and all 8 XMM argument registers (xmm0–xmm7) to a 176-byte register-save area (48 GP + 128 XMM, 16-byte aligned); `__builtin_va_start` initializes all four `va_list` fields (gp_offset, fp_offset, overflow_arg_area, reg_save_area) per the SysV AMD64 ABI; `__builtin_va_end` is a no-op; `va_arg` extraction for GP types (int, unsigned int, long, unsigned long, long long, unsigned long long, pointer) and `double` (via fp_offset into the XMM save area); `va_copy` copies the 24-byte `va_list` struct via three 8-byte moves; `va_arg(ap, float)` is rejected (float is promoted to double in variadic calls per C99 §6.5.2.2p6). Code generation emits `mov al, <xmm_count>` before variadic calls to set the SSE register count per the SysV AMD64 ABI. Callee-saved `rbx` is preserved in every function prologue and epilogue per the SysV AMD64 ABI.
- **Pointers**: pointer types, `&` and `*` as rvalue and lvalue,
  assignment through pointer, pointer parameters and return types,
  `NULL` as a null pointer constant. The address-of operator `&` accepts
  any addressable lvalue: identifiers, array subscripts, struct/union
  member access via `.` (e.g. `&s.x`) and `->` (e.g. `&p->x`).
  Pointer relational comparisons (`<`, `<=`, `>`, `>=`) are supported
  in addition to equality comparisons (`==`, `!=`).
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
  to other functions.
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
  string-literal initialization for pointer globals, brace-list initialization for array globals,
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
  the byte size of the operand's type as a `long` constant. The
  non-parenthesized form `sizeof expr` is also supported. Supported
  scalar types: `char` (1), `short` (2), `int` (4), `long` (8), and
  any pointer type (8). Struct types return the total byte size based on
  natural-alignment field layout. For expression operands, integer promotions and
  usual arithmetic conversions apply to determine the result type
  (`sizeof(char_var + 1)` == 4 because char promotes to int). The
  expression operand is never evaluated — side effects such as `x++`,
  `x = 5`, or function calls inside `sizeof` are suppressed.
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

Anonymous struct/union members (C11 feature), bit-fields; block-scope `extern`;
compound literals at file scope; pointer-to-function-pointer and function-returning-function-pointer;
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

The runner aggregates per-suite results and prints a final
`Aggregate: P passed, F failed, T total` line. As of stage 122 all 1894 tests pass (1210 valid, 260 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).

Individual suites can be run directly, e.g. `./test/valid/run_tests.sh`.

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
