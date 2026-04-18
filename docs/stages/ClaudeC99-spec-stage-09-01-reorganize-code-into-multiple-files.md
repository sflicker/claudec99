# Claude C99 Stage 9.1

## Task
Reorganize the compiler project from a single source file into multiple `.c` and `.h` files grouped by responsibility.

## Goal
The project currently keeps nearly all compiler logic in a single file, `compiler.c`.  
In this stage, restructure the codebase into multiple source and header files so the project is easier to maintain, extend, and review.

This stage is a refactoring stage only. The compiler’s behavior should remain the same after the reorganization.

---

## Requirements

### 1. Split the existing source into multiple modules
Refactor the current implementation into separate files based on functionality.

A suggested target structure is:

#### Source files
- `src/compiler.c`  
  Contains the `main()` function and top-level program flow only.

- `src/ast.c`  
  Contains AST-related helpers and functionality.

- `src/lexer.c`  
  Contains tokenization / lexical analysis logic.

- `src/parser.c`  
  Contains parsing logic.

- `src/codegen.c`  
  Contains code generation logic.

- `src/ast_pretty_printer.c`  
  Contains AST pretty-printing logic.

- `src/util.c`  
  Contains shared utility functions, including file I/O helpers.

#### Header files
Create an `include/` directory and add header files as needed, such as:

- `include/token.h`
- `include/ast.h`
- `include/lexer.h`
- `include/parser.h`
- `include/codegen.h`
- `include/ast_pretty_printer.h`
- `include/util.h`

The exact contents of each file may be adjusted as needed during implementation.

---

### 2. Use headers only for shared declarations
Header files should contain only the declarations and definitions that must be shared across modules.

This may include:
- shared `struct` definitions
- shared `enum` definitions
- typedefs
- function declarations needed by other translation units

Do **not** place internal-only helper declarations in headers unless they are required outside the module.

Module-private helper functions should remain private to their `.c` file, typically by marking them `static`.

---

### 3. Keep responsibilities separated
The reorganization should improve module boundaries.

In particular:
- `compiler.c` should become a thin entry point
- lexer code should live in lexer-related files
- parser code should live in parser-related files
- code generation code should live in codegen-related files
- AST printing code should live in its own module
- general-purpose helpers and file loading utilities should live in `util.c`

Avoid leaving large unrelated sections in `compiler.c`.

---

### 4. Update the build system
Update `CMakeLists.txt` so the project builds correctly with the new file layout.

This includes:
- adding all new source files to the build
- configuring the include path for the `include/` directory
- preserving the existing executable target name unless there is a clear reason to change it

---

### 5. Preserve existing behavior
This stage is structural only.

The refactor must not intentionally change:
- the compiler command-line interface
- parsing behavior
- AST output behavior
- code generation behavior
- pretty-print behavior
- test results

Any code changes beyond what is required for the reorganization should be avoided.

---

## Implementation Notes

### File layout
A typical resulting layout may look like this:

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
