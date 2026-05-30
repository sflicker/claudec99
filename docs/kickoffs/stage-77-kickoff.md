# Stage 77 Kickoff: Enum and Constant Expressions in Case Labels

## Summary of the Spec

Stage 77 adds support for enum constants and simple constant expressions in `case` labels of switch statements. Currently, the compiler only supports integer literals in case labels. This stage enables idiomatic code like:

```C
typedef enum {
    AST_TRANSLATION_UNIT,
    AST_FUNCTION_DECL
} ASTNodeType;

switch (nodeType) {
    case AST_TRANSLATION_UNIT:
        return 42;
    case AST_FUNCTION_DECL:
        return 1;
}
```

**Supported case expressions** (evaluated at compile time):
- Integer literals: `42`
- Character literals: `'A'`
- Enum constants: `AST_TRANSLATION_UNIT`, `TOKEN_NAME`
- Unary `+`, `-`: `+TOKEN_INT`, `-TOKEN_INT`
- Binary `+`, `-`: `BASE + 1`, `TOKEN_INT - 1`

The evaluator must determine if a case label expression is a compile-time constant. Any non-constant value (variable, const variable, etc.) must be rejected with a parser error.

**Out of scope**: Parentheses, multiplication, division, shifts, sizeof, casts, conditionals, and any complex conversion rules.

**Spec issue noted**: In the "Enum in a struct" test, the code shows `default 0;` which is invalid C syntax (missing colon). This will be implemented as `default: return 0;`.

## Tokenizer Changes

None required. No new tokens needed.

## Parser Changes

**File**: `src/parser.c` — function `parse_switch_statement` (case branch)

The parser currently expects `TOKEN_CASE` followed by `TOKEN_INT_LITERAL`. With this stage, it must parse and evaluate a constant expression instead.

**Implementation approach**:
1. Add three helper functions for compile-time evaluation:
   - `eval_case_const_primary()`: Handle integer literals, character literals, and enum constants
   - `eval_case_const_unary()`: Handle unary `+` and `-` applied to a primary
   - `eval_case_const_expr()`: Handle binary `+` and `-` combining unary expressions (left-associative)

2. Modify the `TOKEN_CASE` branch in `parse_switch_statement`:
   - Call `eval_case_const_expr()` instead of `parser_expect(TOKEN_INT_LITERAL)`
   - If evaluation succeeds, the function returns the computed integer value
   - If evaluation fails (non-constant expression), emit `PARSER_ERROR` with message "case label expression is not an integer constant expression"
   - Create an `AST_INT_LITERAL` child node with the computed value as a decimal string

**Evaluation rules**:
- Integer literals and character literals are directly converted to their integer values
- Enum constants must be resolved via symbol table lookup (already available in the parser)
- Unary operators (`+`, `-`) are applied to the primary expression
- Binary `+` and `-` are left-associative and combine two unary expressions
- Any token that does not fit these categories causes an error

## AST Changes

None required. The case value child remains `AST_INT_LITERAL` with the decimal string representation of the computed value. No new node types needed.

## Code Generation Changes

None required. The existing code generation for switch statements already handles negative values correctly via the `cmp eax, %s` instruction. The evaluator outputs the correct integer (negative or positive), and codegen uses it as-is.

## Test Plan

### Valid Tests (6 new)

#### Test 1: test_switch_enum_basic
Basic switch with enum constant.
```C
typedef enum {
    AST_TRANSLATION_UNIT,
    AST_FUNCTION_DECL
} ASTNodeType;

int main() {
    ASTNodeType nodeType = AST_TRANSLATION_UNIT;
    
    switch (nodeType) {
        case AST_TRANSLATION_UNIT:
            return 42;           // expect 42
        case AST_FUNCTION_DECL:
            return 1;
        default:
            return 2;
    }
}
```

#### Test 2: test_switch_binary_add
Binary addition in case label.
```C
#define BASE 0

int main() {
    int x = 2;
    
    switch(x) {
        case BASE:
            return 0;
        case BASE + 1:
            return 1;
        case BASE + 2:
            return 42;      // expect 42
        default:
            return 3;
    }
}
```

#### Test 3: test_switch_unary_neg
Unary negation in case label.
```C
int main() {
    int x = -3;
    
    switch(x) {
        case 0:
            return 0;
        case -3:
            return 42;    // expect 42
        default:
            return 1;
    }
}
```

#### Test 4: test_switch_enum_in_struct
Enum constant from struct member type.
```C
typedef enum {
    AST_TRANSLATION_UNIT,
    AST_FUNCTION_DEF,
    AST_RETURN_STMT
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
} ASTNode;

int classify(ASTNode * node) {
    switch(node->type) {
        case AST_TRANSLATION_UNIT:
            return 10;
        case AST_FUNCTION_DEF:
            return 20;
        case AST_RETURN_STMT:
            return 12;
        default:
            return 0;
    }
}

int main() {
    ASTNode node;
    node.type = AST_RETURN_STMT;
    return classify(&node);      // expect 12
}
```

#### Test 5: test_switch_enum_explicit_values
Enum with explicit assigned values.
```C
enum TokenKind {
    TOKEN_EOF = 0,
    TOKEN_INT = 10,
    TOKEN_NAME = 42
};

int main(void) {
    int kind;
    kind = TOKEN_NAME;
    
    switch (kind) {
        case TOKEN_EOF:
            return 1;
        case TOKEN_INT:
            return 2;
        case TOKEN_NAME:
            return 42;     // expect 42
    }
    return 0;
}
```

#### Test 6: test_switch_char_literal
Character literal in case label.
```C
int main(void) {
    char c;
    c = 'A';
    
    switch(c) {
        case 'A':
            return 42;   // expect 42
        default:
            return 0;
    }
}
```

### Invalid Tests (1 new)

#### Test 1: test_switch_variable_case
Non-constant variable in case label (should fail at parse time).
```C
int main() {
    int x = 10;
    int y = 20;
    int a = 10;
    
    switch (a) {
        case x:              // INVALID: x is not a compile-time constant
            return 1;
        default:
            return 0;
    }
    return 0;
}
```

**Expected**: Compilation error with message "case label expression is not an integer constant expression".

### Pretty Printer Test (1 new)

Add a `test_print_ast_switch_expr` test to verify pretty-printer output for a switch statement with a constant expression in a case label:
```C
int main(void) {
    int x = 5;
    
    switch(x) {
        case 1 + 4:
            return 42;
        default:
            return 0;
    }
}
```

## Implementation Order

1. Update `src/parser.c` — add three evaluator functions and modify `parse_switch_statement` case branch
2. Update `src/version.c` from `"00760000"` to `"00770000"`
3. Create integration test files (6 valid + 1 invalid + 1 pretty-printer)
4. Update `docs/grammar.md` to document the case constant expression rules
5. Update `README.md` if needed (feature list or status table)

## Ambiguities and Decisions

- **Scope of evaluation**: All case expressions must be evaluated at compile time. The evaluator is invoked during parsing, not code generation.

- **Enum constant resolution**: The parser already maintains a symbol table. When encountering an identifier in a case label, the evaluator must look it up and retrieve its constant value. If not found or not an enum constant, it is a non-constant expression error.

- **Operator precedence**: Unary operators bind tighter than binary. The grammar is:
  ```
  case_const_expr   := case_const_unary (('+' | '-') case_const_unary)*
  case_const_unary  := ('+' | '-')* case_const_primary
  case_const_primary := INT_LITERAL | CHAR_LITERAL | IDENTIFIER
  ```

- **Character literal handling**: Character literals like `'A'` are converted to their ASCII value (65) at evaluation time.

- **Negative number representation**: The evaluator produces signed integers. Negative case values (from unary `-` or subtraction) are correctly represented. Code generation uses these values as-is in comparisons.

- **Spec issues identified**:
  1. "Enum in a struct" test shows `default 0;` which is invalid C. Interpreted as requiring a proper default case; the test will use `default: return 0;` instead.
  2. All other test cases in the spec are syntactically valid once written as complete test programs.

- **Error reporting**: When a case label contains a non-constant expression, the parser emits `PARSER_ERROR("case label expression is not an integer constant expression")` and continues, allowing multiple errors to be reported in one pass if needed.
