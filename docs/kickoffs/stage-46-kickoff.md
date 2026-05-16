# Stage 46 Kickoff: Command-Line Arguments

## Summary of the Spec

Stage 46 adds support for command-line arguments passed to `main(int argc, char **argv)`. The integration test verifies that:

1. The compiler correctly parses `main(int argc, char **argv)` function signature
2. Runtime arguments can be accessed via the `argv` array
3. `puts()` can output an argument value
4. Program exit code can be set based on `argc`

The test case: Pass `"Hello"` as a command-line argument, invoke `puts(argv[1])` in main, and verify:
- Output: `Hello` (followed by newline from puts)
- Exit code: 2 (returned by main, which returns argc)

**Spec issues noted:**
- Line 14 has a typo: `arg[1]` should be `argv[1]`
- Line 14 annotation `// Expect Output: "Hello"` is misleading; puts() outputs `Hello\n` without quotes
- Inline annotations (`// ARGS:` and `// EXPECT Exit Code:`) are spec notation only; the harness uses sidecar files (`.args`, `.expected`, `.status`)

## Required Tokenizer Changes

None. The syntax for `main(int argc, char **argv)` is already tokenizable with existing lexer support.

## Required Parser Changes

None. The parser already correctly handles function parameters including pointer-to-pointer types and multiple parameters.

## Required AST Changes

None. The AST already represents `argc` and `argv` parameters correctly.

## Required Code Generation Changes

None. The code generator already correctly passes command-line arguments through `argc` and `argv` when main is invoked by the harness. Existing code generation for array subscript operations (`argv[1]`) and function calls (`puts()`) works without modification.

## Test Plan

Create integration test with sidecar files:
- `test/integration/test_argv_puts.c` — C source file with main(int argc, char **argv)
- `test/integration/test_argv_puts.args` — Single line: `Hello`
- `test/integration/test_argv_puts.expected` — Single line: `Hello`
- `test/integration/test_argv_puts.status` — Single line: `2`

The test harness:
1. Compiles `test_argv_puts.c`
2. Runs the compiled program with argv[1] = "Hello"
3. Captures stdout and exit code
4. Asserts stdout == "Hello\n" (puts adds newline)
5. Asserts exit code == 2 (main returns argc)

## Key Decision

**No compiler changes needed.** The ClaudeC99 compiler already has all necessary functionality to handle command-line arguments. This is a test-only stage. The integration test validates the end-to-end behavior when the compiled program is invoked by the harness with command-line arguments.

---

Implementation will be test-only, with no changes to tokenizer, parser, AST, or code generation phases.
