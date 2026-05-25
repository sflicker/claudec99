# Stage 70 Kickoff: Mini Compiler-Shaped Integration Test

## Summary of the Spec

Stage 70 adds a single integration test called `test_mini_compiler` that exercises many existing compiler features together in a realistic "mini compiler" pattern. No new language features are added.

The test program is a self-contained C program that demonstrates a simple tokenizer similar to patterns used in the main compiler codebase. It:
- Tokenizes a hardcoded input string: `"result + 42 * x - 7"`
- Uses an enum to represent token kinds (identifier, number, operator, EOF)
- Stores tokens in a struct-based representation
- Uses a dynamic heap array (malloc/realloc/free) for token storage
- Demonstrates string/pointer arithmetic, switch statements, memory management, and printf formatting

The goal is not to create a fully functioning compiler, but rather to stress-test existing features in a realistic, compiler-like pattern.

## Required Tokenizer Changes

None. All token types and tokenization logic needed are already supported by the existing tokenizer.

## Required Parser Changes

None. The test program itself is self-contained and does not require new syntactic features in the C language being compiled.

## Required AST Changes

None. No AST modifications are needed.

## Required Code Generation Changes

None. The test exercises existing codegen for structs, enums, pointers, function calls, and control flow.

## Test Plan

### Integration Test: test_mini_compiler

**Location:** `test/integration/test_mini_compiler/`

**Files to create:**
- `test_mini_compiler.c` — The main test program containing the mini tokenizer
- `test_mini_compiler.expected` — Expected output from successful tokenization and display

**Test scope:**
- Declare an enum for token kinds (TK_IDENT, TK_NUMBER, TK_PLUS, TK_MINUS, TK_MUL, TK_EOF)
- Declare a struct to hold individual tokens (kind, value for identifiers/numbers)
- Implement a tokenizer function that scans the input string and populates a heap array of tokens
- Use malloc/realloc to grow the token array as needed
- Use memcpy to store string data (if applicable)
- Use pointer arithmetic for string traversal
- Use switch statements to classify characters
- Use printf to output the tokenized results
- Call free to clean up allocated memory
- Verify all tokens are correctly identified and displayed

### Expected Behavior

The program should:
1. Parse the input string `"result + 42 * x - 7"`
2. Correctly identify:
   - `result` as an identifier
   - `+` as an operator (PLUS)
   - `42` as a number
   - `*` as an operator (MUL)
   - `x` as an identifier
   - `-` as an operator (MINUS)
   - `7` as a number
   - EOF at the end
3. Print formatted output showing each token and its details
4. Return 0 (success)

### Verification Steps

1. Compile `test_mini_compiler.c` using the ClaudeC99 compiler
2. Run the compiled executable
3. Compare output against `test_mini_compiler.expected`
4. Verify all memory is properly allocated and freed (no leaks in the logic)

## Implementation Notes

- The test program is self-contained and does not read external files; it uses string literals for simplicity
- The tokenizer uses a simple state machine (switch-based) to identify token types
- The heap array uses standard C reallocation patterns for growth
- All existing features are used as-is; no new language constructs are required
