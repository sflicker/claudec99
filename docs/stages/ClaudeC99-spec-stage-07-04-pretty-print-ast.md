# Claude C99 Stage 7.4

Task: Add a command line option that will generate a pretty printed AST as part of the output.

## Requirements:
- Add support for the following command line option:
  - --print-ast

## Behavior
- The compiler must parse the input program as usual.
- if --print-ast is provided:
  - After successful parsing, the compiler must print a pretty-formatted representation of the AST.
  - The compiler must then exit successfully.
  - No code generation or further compilation stages should be preformed.
- if --print-ast is not provided
  - The compiler should exactly as in the previous stage.

## Error handling
- If parsing fails:
  - The compiler must report the error as usual.
  - No AST output should be printed.

## AST Pretty print format
The AST must be printed in a hierarchical, human-readable format.

## Formatting rules
- Each AST node must appear on its own line.
- Child node must be indented beneath their parent node.
- Indentation must use spaces (recommended: 2 spaces per level).
- The structure of the output must clearly reflect the tree hierarchy.

## Node representation
- Each node should be printed using a descriptive label.
- When applicable, include relevant values such as:
  - Function names
  - Variable names
  - Literal values
  - Operator types
- Operators should be printed using readable names such as:
  - `ADD`, `SUBTRACT`, `LESSTHAN`, etc.

## Example 
Given the input program

`int main() {
    int a = 0;
    while (a < 42) {
        a = a + 1;
    }
    return a;
}
`

The AST output should be similar to 
`ProgramDecl:
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
`
## Notes
- The exact whitespace does not need to match this example exactly, but:
  - The indentation must be consistent.
  - The hierarchical structure must be clear.
- The node labels and general structure shown above should be treated as the expected format for testing.

## Scope
- This stage only introduces AST pretty-printing.
- Do not modify the grammar or parsing behavior.
- Do not introduce new language features.
- No not modify code generation logic beyond conditionally skipping it when --print-ast is used.

## Implementation guidelines
Implement this stage in the following order:
1. Add command line parsing support for `--print-ast`
2. Implement an AST pretty-printing function
3. Integrate the pretty printer to run after successful parsing.
4. Ensure the compiler exits after print when the option is enabled.
5. Add tests to verify correct AST output

## Testing Guidelines:
- Add tests that:
  - Run the compiler with `--print-ast`
  - Compare the output against the expected AST structures
- Ensure tests cover:
  - Variable declarations
  - Expressions
  - Control flow constructs (if, while, for)
  - Nested blocks
- Ensure:
  - No assembly output is generated when `--print-ast` is used
  - Output is deterministic and stable for comparison

