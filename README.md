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
| `self-host` | Full C0→C1→C2 cycle: archives old named binaries to `build/previous/`, saves current `build/ccompiler` as `ccompiler-c0`, bootstraps to `ccompiler-c1` (runs test suite, commits checkpoint), bootstraps to `ccompiler-c2` (runs test suite). `build/ccompiler` is left as C2. |
| `fallback` | Uses `build/ccompiler` if present, otherwise normal cmake build. |

`--timeout=N` sets the per-file compilation timeout for bootstrap builds (default: 300 s).

The standard stage workflow is:

```
./build.sh --mode=normal      # implementation phase: GCC build + tests
./build.sh --mode=self-host   # self-host phase: C0→C1→C2 cycle with tests
```

## Compiler limits

All hard-coded capacity limits live in `include/constants.h` as object-like
macros. Redefine any of them on the compiler command line (e.g.
`-DPARSER_MAX_FUNCTIONS=256`) or edit the file directly before building to
raise a limit.

### String buffers

| Constant | Default | Description |
|----------|---------|-------------|
| `MAX_NAME_LEN` | 256 | Maximum byte length of any identifier, tag name, assembly label, or value string stored in a fixed-size `char` array throughout the compiler. All `char name[]`, `char tag[]`, `char label[]`, and `char value[]` fields in structs use this size. |

### AST

| Constant | Default | Description |
|----------|---------|-------------|
| `AST_MAX_CHILDREN` | 64 | Initial capacity of an `ASTNode`'s child array. The array is allocated lazily and grows by doubling, so this is a starting size rather than a hard cap (parameters in a call expression, fields in an initializer list, statements in a block, and top-level declarations in a translation unit all grow as needed). |

### Type system

| Constant | Default | Description |
|----------|---------|-------------|
| `FUNC_TYPE_MAX_PARAMS` | 16 | Maximum number of parameter types stored on a `TYPE_FUNCTION` type node (used for function-pointer types). |

### Parser

| Constant | Default | Description |
|----------|---------|-------------|
| `PARSER_MAX_FUNCTIONS` | 256 | Maximum number of function declarations/definitions in a translation unit. |
| `PARSER_MAX_GLOBALS` | 256 | Maximum number of file-scope object declarations in a translation unit. |
| `PARSER_MAX_TYPEDEFS` | 64 | Maximum number of typedef entries visible at any point during parsing. |
| `PARSER_MAX_ENUM_CONSTS` | 256 | Maximum number of enum constants across all enum declarations in a translation unit. |
| `PARSER_MAX_ENUM_TAGS` | 32 | Maximum number of distinct named enum tags. |
| `PARSER_MAX_STRUCT_TAGS` | 32 | Maximum number of distinct named struct tags. |
| `PARSER_MAX_UNION_TAGS` | 32 | Maximum number of distinct named union tags. |
| `PARSER_MAX_STRUCT_FIELDS` | 64 | Maximum number of fields in a single struct or union definition. |
| `FUNC_MAX_PARAMS` | 16 | Maximum number of parameters in a function declaration or definition. |

### Code generator

| Constant | Default | Description |
|----------|---------|-------------|
| `MAX_LOCALS` | 256 | Maximum number of local variables per function. |
| `MAX_GLOBALS` | 256 | Maximum number of global variables tracked by the code generator. |
| `MAX_BREAK_DEPTH` | 32 | Maximum nesting depth of breakable constructs (loops and switches). |
| `MAX_SWITCH_DEPTH` | 16 | Maximum nesting depth of `switch` statements. |
| `MAX_SWITCH_LABELS` | 256 | Maximum number of `case`/`default` labels in a single `switch`. |
| `MAX_USER_LABELS` | 64 | Maximum number of user-defined `goto` labels per function. |
| `MAX_STRING_LITERALS` | 2048 | Maximum number of string literal occurrences in a translation unit. |
| `MAX_LOCAL_STATICS` | 128 | Maximum number of block-scope `static` variables across all functions in a translation unit. |

### Preprocessor

| Constant | Default | Description |
|----------|---------|-------------|
| `MAX_INCLUDE_DEPTH` | 64 | Maximum `#include` nesting depth. |
| `MAX_COND_DEPTH` | 64 | Maximum nesting depth of `#if`/`#ifdef`/`#ifndef` conditional blocks. |

## Usage

```
ccompiler [--version] [--print-ast | --print-tokens] [-Werror] [--max-errors=N] [--sysroot=<dir>] [-DNAME[=VAL]] [-I<dir>] <source.c>
```

- Default: writes `<name>.asm` next to the invocation directory and
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
    `#if`/`#elif` (below), with first-true-wins branch semantics. Inactive
    regions are fully skipped — not emitted, not macro-expanded, and nested
    `#define`/`#include` suppressed. Nesting up to 64 levels deep. Errors on a
    missing `#endif`, unmatched `#else`/`#endif`, duplicate `#else`, `#elif`
    without a conditional, and `#elif` after `#else`.
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
  with unary/binary `+`/`-` operators), `break`, `continue`, `goto`/labels, block scopes with shadowing.
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
  Enum declarations (named and anonymous) with auto-incrementing or explicit literal (integer/character) values;
  enum constants are available as compile-time integer values throughout the translation unit.
  Block-scope `static` variables (scalar and pointer types) persist values across function calls and are stored in .bss or .data with constant-only initializers.
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
  Variadic function declarations and definitions (e.g., `int f(int x, ...)`) with caller compatibility checking (actual args >= fixed params); callee-side access to extra arguments via `va_list`, `va_start`, `va_end`, `<stdarg.h>`: variadic function prologues save all 6 GP argument registers (rdi–r9) to a hidden 304-byte register save area; `__builtin_va_start` initializes all four `va_list` fields (gp_offset, fp_offset, overflow_arg_area, reg_save_area) per the SysV AMD64 ABI; `__builtin_va_end` is a no-op; `va_arg` extraction for GP register class types (int, unsigned int, long, unsigned long, long long, unsigned long long, and pointer types) including register-save area and overflow stack paths; `va_copy` remains a stub. Code generation emits `xor eax, eax` before variadic calls to satisfy the SysV AMD64 ABI float-argument-count protocol.
- **Pointers**: pointer types, `&` and `*` as rvalue and lvalue,
  assignment through pointer, pointer parameters and return types,
  `NULL` as a null pointer constant. The address-of operator `&` accepts
  any addressable lvalue: identifiers, array subscripts, struct/union
  member access via `.` (e.g. `&s.x`) and `->` (e.g. `&p->x`).
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
  pointers, arrays, structs), both initialized (with constant integer expressions,
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
  pointer. `sizeof(int[10])` (array-type-name form) is not yet supported.
  `const`-qualified type names in `sizeof` are supported (e.g.,
  `sizeof(const int)`, `sizeof(const char *)`); the qualifier does not
  affect the computed size.
- **Conditional operator**: `condition ? expr_true : expr_false`. The condition is evaluated first; only the selected branch (true or false) is then evaluated and its value returned. The condition may be any integer or pointer expression. Both branches may be integer expressions (result is common type) or compatible pointer types (result is that pointer type). One branch may be a pointer with the other being the null constant `0`. The conditional expression is right-associative with lower precedence than logical OR and higher precedence than assignment.
- **Comma operator**: `expr1, expr2` evaluates both expressions left to right, discards the left result, and returns the right result. Comma is the lowest-precedence operator (below assignment), left-associative, and produces an rvalue. Comma-as-separator in function calls and initializers is preserved via parser-level precedence.

## Not yet supported

Anonymous struct/union members (C11 feature), bit-fields; floating-point; block-scope `extern`; block-scope `static` arrays and structs;
floating-point variadic arguments; `va_copy` (va_start/va_end and va_arg extraction for GP types are now implemented);
`#elifdef`/`#elifndef`; pointer-to-function-pointer and function-returning-function-pointer.

The authoritative grammar for the supported language is in
[`docs/grammar.md`](docs/grammar.md). The full per-feature checklist is in
[`docs/outlines/checklist.md`](docs/outlines/checklist.md).

## Tests

The test harness consists of six suites under `test/`:

| Suite          | What it checks                                                      |
| -------------- | ------------------------------------------------------------------- |
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
`Aggregate: P passed, F failed, T total` line. As of stage 93 all tests pass (823 valid, 237 invalid, 82 integration, 43 print-AST, 100 print-tokens, 21 print-asm; 1306 total).

Individual suites can be run directly, e.g. `./test/valid/run_tests.sh`.

Tests in `test/valid/` use the naming convention
`test_<description>__<expected_exit_code>.c` so the runner can extract
the expected exit code from the filename.

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
