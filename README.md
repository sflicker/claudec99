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

Through stage 15-01 (character literal lexer support):

- **Statements**: `if/else`, `while`, `do/while`, `for`, `switch/case/default`,
  `break`, `continue`, `goto`/labels, block scopes with shadowing, `//` and
  `/* */` comments.
- **Integer types**: `char`, `short`, `int`, `long` with usual promotions,
  conversions, and explicit casts. Integer literals with `L` suffix.
- **Functions**: multiple functions per translation unit, forward
  declarations, SysV AMD64 calls with up to 6 arguments, typed parameter
  and return-type conversions at the call boundary, calls into libc
  via `extern` emission for declared-but-not-defined functions
  (e.g. `int puts(char *s);`).
- **Pointers**: pointer types, `&` and `*` as rvalue and lvalue,
  assignment through pointer, pointer parameters and return types,
  `NULL` as a null pointer constant.
- **Arrays**: array declarations, indexing, array-to-pointer decay,
  pointer arithmetic, subscript-as-pointer-arithmetic, initialization
  of local `char` arrays from a string literal (with explicit or
  inferred size).
- **String literals**: tokenization, AST node, static-data emission,
  decay to `char *` in expressions, decoded escape sequences (`\n`,
  `\t`, `\r`, `\\`, `\"`, `\0`).
- **Character literals (lexer only)**: tokenization of `'A'`, `'0'`,
  and the same escape set extended with `\'` (`\n`, `\t`, `\r`, `\\`,
  `\'`, `\"`, `\0`). Parser/AST/codegen support is not yet wired up.

## Not yet supported

Structs, unions, enums; floating-point and unsigned types; `typedef`
and storage-class specifiers; variadics; preprocessor; function
pointers; calls with more than 6 arguments. Initializer lists for
non-`char` arrays and global string-literal array initialization are
not yet supported.

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
`Aggregate: P passed, F failed, T total` line. As of stage 15-01 all
417 tests pass (252 valid, 60 invalid, 23 print-AST, 74 print-tokens,
8 print-asm).

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
