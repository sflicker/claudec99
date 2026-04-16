# Stage 7.4 Transcript: Pretty-Print AST

## Summary

Added a `--print-ast` command-line option to the ClaudeC99 compiler, as
defined in `docs/stages/ClaudeC99-spec-stage-07-04-pretty-print-ast.md`.

When `--print-ast` is provided, the compiler parses the input program as
usual, prints a hierarchical, human-readable representation of the AST to
stdout, and exits successfully. No code generation or assembly output is
produced. When the flag is omitted, the compiler behaves identically to
the previous stage.

## Changes Made

### Step 1: Command-Line Argument Parsing

Updated `main()` to support an optional `--print-ast` flag. The previous
implementation required exactly 2 arguments (`argc != 2`). The new
implementation loops over `argv`, recognizing `--print-ast` as a flag and
treating the remaining argument as the source file:

```c
int print_ast = 0;
const char *source_file = NULL;

for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--print-ast") == 0) {
        print_ast = 1;
    } else if (!source_file) {
        source_file = argv[i];
    } else {
        fprintf(stderr, "usage: ccompiler [--print-ast] <source.c>\n");
        return 1;
    }
}
```

The flag can appear before or after the source file path. After parsing
succeeds, if `print_ast` is set, the compiler calls the pretty-printer
and returns 0 immediately, skipping code generation.

### Step 2: AST Pretty Printer (New Section 5)

Added three functions in a new section between the parser and code
generator:

**`operator_name()`** — maps raw operator strings to readable names:

| Operator | Name                  |
|----------|-----------------------|
| `+`      | `ADD`                 |
| `-`      | `SUBTRACT`            |
| `*`      | `MULTIPLY`            |
| `/`      | `DIVIDE`              |
| `==`     | `EQUAL`               |
| `!=`     | `NOTEQUAL`            |
| `<`      | `LESSTHAN`            |
| `<=`     | `LESSTHANOREQUAL`     |
| `>`      | `GREATERTHAN`         |
| `>=`     | `GREATERTHANOREQUAL`  |
| `!`      | `NOT`                 |
| `++`     | `INCREMENT`           |
| `--`     | `DECREMENT`           |

**`ast_print_indent()`** — prints `depth * 2` spaces for indentation.

**`ast_pretty_print()`** — recursively walks the AST tree. Each node type
maps to a descriptive label:

| AST Node Type        | Output Label           |
|----------------------|------------------------|
| `AST_PROGRAM`        | `ProgramDecl:`         |
| `AST_FUNCTION_DECL`  | `FunctionDecl: <name>` |
| `AST_BLOCK`          | `Block`                |
| `AST_DECLARATION`    | `VariableDeclaration: <name>` |
| `AST_RETURN_STATEMENT` | `ReturnStmt:`        |
| `AST_INT_LITERAL`    | `IntLiteral: <value>`  |
| `AST_VAR_REF`        | `VariableExpression: <name>` |
| `AST_BINARY_OP`      | `Binary: <OP_NAME>`   |
| `AST_UNARY_OP`       | `Unary: <OP_NAME>`    |
| `AST_ASSIGNMENT`     | `Assignment: <name>`  |
| `AST_EXPRESSION_STMT`| `ExpressionStatement`  |
| `AST_IF_STATEMENT`   | `IfStmt:`              |
| `AST_WHILE_STATEMENT`| `WhileStmt:`           |
| `AST_FOR_STATEMENT`  | `ForStmt:`             |
| `AST_PREFIX_INC_DEC` | `PrefixIncDec: <OP_NAME>` |
| `AST_POSTFIX_INC_DEC`| `PostfixIncDec: <OP_NAME>` |

Children are printed at `depth + 1`. The `AST_FOR_STATEMENT` case has
special handling: it iterates over its 4 children (init, condition,
update, body) and skips any that are `NULL` (representing omitted
clauses), then returns early to avoid the generic child loop.

### Example Output

Given:
```c
int main() {
    int a = 0;
    while (a < 42) {
        a = a + 1;
    }
    return a;
}
```

Output:
```
ProgramDecl:
  FunctionDecl: main
    Block
      VariableDeclaration: a
        IntLiteral: 0
      WhileStmt:
        Binary: LESSTHAN
          VariableExpression: a
          IntLiteral: 42
        Block
          ExpressionStatement
            Assignment: a
              Binary: ADD
                VariableExpression: a
                IntLiteral: 1
      ReturnStmt:
        VariableExpression: a
```

### Step 3: Tests

Created `test/print_ast/` with 6 test cases and a runner script:

| Test File                     | Coverage                          |
|-------------------------------|-----------------------------------|
| `test_variable_declaration.c` | Variable declaration with init    |
| `test_expressions.c`         | Binary ops with operator precedence |
| `test_if_else.c`             | If/else control flow              |
| `test_while_loop.c`          | While loop (matches spec example) |
| `test_for_loop.c`            | For loop with all three clauses   |
| `test_nested_blocks.c`       | Nested if blocks                  |

Each test has a `.c` input file and a `.expected` file containing the
exact expected AST output. The runner (`run_tests.sh`) invokes the
compiler with `--print-ast`, compares output against the expected file,
and also verifies no `.asm` file is generated.

### What Did NOT Change

- **Tokenizer**: No changes.
- **Parser**: No changes.
- **AST node types**: No changes — the printer reads existing node types.
- **Code generation**: No logic changes — only conditionally skipped in
  `main()` when `--print-ast` is set.
- **Existing tests**: All 69 valid tests and 2 invalid tests continue to
  pass.

## Test Results

```
-- print_ast tests --
PASS  test_expressions
PASS  test_for_loop
PASS  test_if_else
PASS  test_nested_blocks
PASS  test_variable_declaration
PASS  test_while_loop
Results: 6 passed, 0 failed, 6 total

-- valid tests --
Results: 69 passed, 0 failed, 69 total

-- invalid tests --
Results: 2 passed, 0 failed, 2 total
```

## Spec Notes

Two minor typos in the spec (did not affect implementation):
- Line 15: "The compiler should exactly as in the previous stage" —
  missing "behave"
- Line 84: "No not modify code generation" — should be "Do not modify"
