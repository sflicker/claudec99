# Stage 9.1 Transcript: Reorganize Code Into Multiple Files

## Summary

Pure refactor of the compiler defined in
`docs/stages/ClaudeC99-spec-stage-09-01-reorganize-code-into-multiple-files.md`.

The 1,182-line monolithic `src/compiler.c` was split into seven `.c`
modules under `src/` with shared declarations placed in seven header
files under a new `include/` directory. **No behavioral change** was
intentional: the CLI, parser, AST output, codegen, and pretty-printer
all behave identically.

The build system was updated to compile all sources and add `include/`
to the include path. The executable target name `ccompiler` was
preserved.

## Plan

The original `compiler.c` was already organized into eight numbered
sections. The mapping into modules was natural:

| Original section            | New header                       | New source                  |
| --------------------------- | -------------------------------- | --------------------------- |
| 1. Token Definitions        | `include/token.h`                | (no .c needed)              |
| 2. AST Node Definitions     | `include/ast.h`                  | `src/ast.c`                 |
| 3. Lexer                    | `include/lexer.h`                | `src/lexer.c`               |
| 4. Parser                   | `include/parser.h`               | `src/parser.c`              |
| 5. AST Pretty Printer       | `include/ast_pretty_printer.h`   | `src/ast_pretty_printer.c`  |
| 6. Code Generator           | `include/codegen.h`              | `src/codegen.c`             |
| 7. File I/O Utilities       | `include/util.h`                 | `src/util.c`                |
| 8. Main                     | (none)                           | `src/compiler.c`            |

### Header / static decisions

Per the spec, headers contain only declarations and definitions that
must cross translation-unit boundaries. Everything else stays
`static` inside its `.c` file.

- **In headers (public):**
  - `Token`, `TokenType`
  - `ASTNode`, `ASTNodeType`, `AST_MAX_CHILDREN`
  - `Lexer`, `Parser`, `CodeGen`, `LocalVar`, `MAX_LOCALS`
  - The functions `main()` calls into other modules: `lexer_init`,
    `lexer_next_token`, `parser_init`, `parse_program`, `ast_new`,
    `ast_add_child`, `ast_free`, `ast_pretty_print`, `codegen_init`,
    `codegen_program`, `read_file`, `make_output_path`.
- **`static` in `.c` (private):**
  - `lexer_skip_whitespace`
  - All `parse_*` helpers (and `parser_expect`) except `parse_program`
  - `operator_name`, `ast_print_indent`
  - All `codegen_*` helpers except `codegen_init`/`codegen_program`
  - `count_declarations`

The `Parser`, `Lexer`, and `CodeGen` structs themselves stay in their
headers because `main()` allocates them on the stack — that requires
the full type definition.

## Changes Made

### Step 1: Header files (`include/`)

Created seven headers, all with `#ifndef CCOMPILER_*_H` include guards.

- `token.h` — defines `TokenType` enum and `Token` struct. No
  dependencies.
- `ast.h` — defines `ASTNodeType` enum, `ASTNode` struct,
  `AST_MAX_CHILDREN`, and declares the three AST helpers.
- `lexer.h` — includes `token.h`; defines `Lexer` and declares
  `lexer_init` / `lexer_next_token`.
- `parser.h` — includes `ast.h`, `lexer.h`, `token.h`; defines
  `Parser` and declares `parser_init` / `parse_program`.
- `codegen.h` — includes `ast.h` and `<stdio.h>` (for `FILE *`);
  defines `LocalVar`, `MAX_LOCALS`, `CodeGen`, and declares
  `codegen_init` / `codegen_program`.
- `ast_pretty_printer.h` — includes `ast.h`; declares
  `ast_pretty_print`.
- `util.h` — includes `<stddef.h>` (for `size_t`); declares
  `read_file` and `make_output_path`.

### Step 2: Small modules (`ast.c`, `util.c`, `lexer.c`)

Moved each section verbatim. Each `.c` file `#include`s its own
header plus only the system headers it actually needs (`<stdio.h>`,
`<stdlib.h>`, `<string.h>`, `<ctype.h>`).

`lexer.c` calls `atoi`, so it includes `<stdlib.h>` (this was missed
on the first write and added immediately).

### Step 3: Larger modules (`parser.c`, `ast_pretty_printer.c`, `codegen.c`)

The parser, pretty-printer, and code generator were each moved into
their own files unchanged. The only edit needed was changing the one
public function in each module from `static` to a normal definition:

- `parse_program` (parser.c)
- `ast_pretty_print` (ast_pretty_printer.c)
- `codegen_init` and `codegen_program` (codegen.c)

All other helpers in those files stayed `static`.

### Step 4: Slim `compiler.c`

Replaced the contents of `src/compiler.c` with a thin entry point: the
file-level comment, the system + project includes, and `main()`. Total
~80 lines, down from 1,182.

```c
#include "ast.h"
#include "ast_pretty_printer.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "util.h"

int main(int argc, char *argv[]) { ... }
```

### Step 5: Build system

Updated `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.10)
project(ccompiler C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(ccompiler
    src/compiler.c
    src/ast.c
    src/ast_pretty_printer.c
    src/codegen.c
    src/lexer.c
    src/parser.c
    src/util.c
)

target_include_directories(ccompiler PRIVATE include)
```

Target name `ccompiler` preserved.

## Verification

Clean configure + build, zero warnings:

```
[ 12%] Building C object CMakeFiles/ccompiler.dir/src/compiler.c.o
[ 25%] Building C object CMakeFiles/ccompiler.dir/src/ast.c.o
[ 37%] Building C object CMakeFiles/ccompiler.dir/src/ast_pretty_printer.c.o
[ 50%] Building C object CMakeFiles/ccompiler.dir/src/codegen.c.o
[ 62%] Building C object CMakeFiles/ccompiler.dir/src/lexer.c.o
[ 75%] Building C object CMakeFiles/ccompiler.dir/src/parser.c.o
[ 87%] Building C object CMakeFiles/ccompiler.dir/src/util.c.o
[100%] Linking C executable ccompiler
[100%] Built target ccompiler
```

All existing test suites pass:

| Suite              | Result       |
| ------------------ | ------------ |
| `test/valid`       | 74 / 74 pass |
| `test/invalid`     | 3 / 3 pass   |
| `test/print_ast`   | 6 / 6 pass   |
| **Total**          | **83 / 83**  |

## Final Layout

```text
project-root/
├── CMakeLists.txt
├── include/
│   ├── ast.h
│   ├── ast_pretty_printer.h
│   ├── codegen.h
│   ├── lexer.h
│   ├── parser.h
│   ├── token.h
│   └── util.h
└── src/
    ├── ast.c
    ├── ast_pretty_printer.c
    ├── codegen.c
    ├── compiler.c
    ├── lexer.c
    ├── parser.c
    └── util.c
```
