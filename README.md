# ClaudeC99

A staged, educational C99 compiler built using Claude Code. The compiler
translates a growing subset of C99 to x86_64 assembly (NASM, ELF64, Linux).

## Pipeline

```
source.c -> Lexer -> Parser (AST) -> Code Generator -> source.asm
```

The compiler is implemented in C11. The output `.asm` file is assembled
with `nasm -f elf64` and linked with `ld -e main` to produce a Linux
executable.

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

```
cmake -S . -B build
cmake --build build
```

This produces `build/ccompiler`.

## Usage

```
ccompiler [--print-ast | --print-tokens] <source.c>
```

- Default: writes `<name>.asm` next to the invocation directory and
  prints `compiled: <source> -> <name>.asm`.
- `--print-tokens`: dumps the token stream and exits.
- `--print-ast`: dumps the AST and exits.

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

Through Stage 30 (struct support):

- **Statements**: `if/else`, `while`, `do/while`, `for`, `switch/case/default`,
  `break`, `continue`, `goto`/labels, block scopes with shadowing, `//` and
  `/* */` comments.
- **Declarations**: comma-separated init-declarator lists (e.g., `int a, b;`,
  `int a=3, b=4;`, `int *p, q;`). Parenthesized declarators provide grouping syntax
  for pointers (e.g., `int (*p)` and `int (**pp)`) with semantics equivalent to
  unparenthesized forms. Typedef aliases for integer scalar types, pointer types
  (e.g., `typedef int *IntPtr;`, `typedef char *String;`), array types (e.g., `typedef int A[4];`),
  and function pointer types (e.g., `typedef int (*BinaryFn)(int, int);`) with full type chain support,
  block-scope tracking, and shadowing. The typedef name can be used as a type specifier in variable
  declarations, assignments, multi-declarator lists, and (for function pointers) indirect calls.
  Enum declarations (named and anonymous) with auto-incrementing or explicit literal (integer/character) values;
  enum constants are available as compile-time integer values throughout the translation unit.
- **Integer types**: `char`, `short`, `int`, `long` with usual promotions,
  conversions, and explicit casts. Integer literals with `L` suffix.
- **Functions**: multiple functions per translation unit, forward
  declarations with compatibility checking (return type and parameter type
  matching between declarations and definitions), SysV AMD64 calls with up to
  6 arguments, typed parameter and return-type conversions at the call boundary,
  calls into libc via `extern` emission for declared-but-not-defined functions
  (e.g. `int puts(char *s);`). `static` functions have internal linkage
  (no `global` NASM directive emitted).
- **Pointers**: pointer types, `&` and `*` as rvalue and lvalue,
  assignment through pointer, pointer parameters and return types,
  `NULL` as a null pointer constant.
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
- **Arrays**: array declarations, indexing, array-to-pointer decay,
  pointer arithmetic, subscript-as-pointer-arithmetic, initialization
  of local `char` arrays from a string literal (with explicit or
  inferred size), file-scope (global) uninitialized array declarations.
- **Structs**: named struct definitions with natural-alignment field layout,
  local and global struct variables, sizeof operator on struct types and struct instances.
  Member access via "." and "->" is not yet supported.
- **File-scope objects**: file-scope (global) object declarations (scalars,
  pointers, arrays, structs), both initialized (with constant integer expressions,
  emitted to `.data`) and uninitialized (with zero-initialization, emitted to
  `.bss`). Duplicate declarations and type mismatches are rejected. Function
  and object names do not collide in the ordinary identifier namespace.
  `extern` and `static` storage class specifiers are supported: `extern` declares
  without allocating storage; `static` declares with internal linkage.
- **String literals**: tokenization, AST node, static-data emission,
  decay to `char *` in expressions, decoded escape sequences (`\n`,
  `\t`, `\r`, `\\`, `\"`, `\0`).
- **Character literals**: tokenization, AST node, and codegen for
  `'A'`, `'0'`, and the full simple-escape set (`\a`, `\b`, `\f`,
  `\n`, `\r`, `\t`, `\v`, `\\`, `\'`, `\"`, `\?`, `\0`). A character
  constant evaluates as an `int` (`'A'` = 65, `'\n'` = 10, etc.) and
  is a valid primary expression in any integer context: returns,
  initializers (`char`/`int`/`long`), assignment, arithmetic,
  comparison, `if` conditions, and as the right-hand side of an
  array element assignment. Malformed character literals
  (empty `''`, multi-character `'ab'`, unknown escapes such as
  `'\q'`, unterminated literals, and raw newline inside a literal)
  are rejected with diagnostic messages.
- **Compound assignment operators**: `+=`, `-=`, `*=`, `/=`, `%=`,
  `<<=`, `>>=`, `&=`, `^=`, `|=` on integer variables. Each
  desugars to `a op= b` → `a = a op b`, so all arithmetic, shift,
  and bitwise rules apply to the right-hand expression.
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
  complement), `++`/`--` (prefix and postfix), `*` (dereference),
  `&` (address-of). `~` and `!` are integer-only: pointer and array
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
- **Conditional operator**: `condition ? expr_true : expr_false`. The condition is evaluated first; only the selected branch (true or false) is then evaluated and its value returned. The condition may be any integer or pointer expression. Both branches may be integer expressions (result is common type) or compatible pointer types (result is that pointer type). One branch may be a pointer with the other being the null constant `0`. The conditional expression is right-associative with lower precedence than logical OR and higher precedence than assignment.
- **Comma operator**: `expr1, expr2` evaluates both expressions left to right, discards the left result, and returns the right result. Comma is the lowest-precedence operator (below assignment), left-associative, and produces an rvalue. Comma-as-separator in function calls and initializers is preserved via parser-level precedence.

## Not yet supported

Struct member access (`.` and `->`), anonymous structs, bit-fields, struct assignment/parameters/return values;
unions; floating-point and unsigned types; array/struct
typedefs (pointer and function-pointer typedefs are now supported); block-scope storage class specifiers;
variadics; preprocessor; pointer-to-function-pointer and function-returning-function-pointer;
calls with more than 6 arguments. Initializer lists for non-`char` arrays and
global string-literal array initialization are not yet supported.

The authoritative grammar for the supported language is in
[`docs/grammar.md`](docs/grammar.md). The full per-feature checklist is in
[`docs/outlines/checklist.md`](docs/outlines/checklist.md).

## Tests

The test harness consists of five suites under `test/`:

| Suite          | What it checks                                                      |
| -------------- | ------------------------------------------------------------------- |
| `valid`        | Compile, assemble, link, run; exit code must match `__N` in filename. If a sibling `<name>.expected` file is present, the program's stdout must also match it byte-for-byte. |
| `invalid`      | Compiler must reject the program                                    |
| `print_ast`    | `--print-ast` output must match `.expected`                         |
| `print_tokens` | `--print-tokens` output must match `.expected`                      |
| `print_asm`    | Generated `.asm` must match `.expected`                             |

Run everything from the project root after building:

```
./test/run_all_tests.sh
```

The runner aggregates per-suite results and prints a final
`Aggregate: P passed, F failed, T total` line. As of stage 30 all
tests pass (457 valid, 141 invalid, 24 print-AST, 88 print-tokens,
21 print-asm; 739 total).

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

- A C11 toolchain and CMake (>= 3.10) to build the compiler.
- `nasm` and `ld` to assemble and link the generated `.asm` files.
- Linux x86_64 to run the produced binaries.
