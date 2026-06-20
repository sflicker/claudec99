# Stage 156 Kickoff: Fix `-O1` Optimizer Bug in Switch Statement Dead-Code Removal

## Summary of the Spec

CC99-014 is a correctness regression where the `-O1` optimizer miscompiles switch-based state machine code. The optimizer produces exit code 4 where `-O0` and GCC both produce 23.

**Root cause**: The dead-code removal pass in `src/optimize.c` only recognizes `AST_LABEL_STATEMENT` as a stopping point when scanning for unreachable statements after an unconditional control-flow statement. It fails to stop at `AST_CASE_SECTION` and `AST_DEFAULT_SECTION`, causing it to incorrectly remove case labels and their bodies as "dead code".

**Example symptom**: In a switch block where a `break` statement appears before a `case` label, the `case` label and all subsequent cases are removed.

## Required Tokenizer Changes

None. The tokenizer is unaffected.

## Required Parser Changes

None. The parser is unaffected.

## Required AST Changes

None. The AST structure is unaffected.

## Required Code Generation Changes

None. Code generation is unaffected.

## Test Plan

### 1. Core Fix
- Modify `src/optimize.c`, function `optimize_stmt`, `AST_BLOCK` case
- In the dead-code scan loop, add stop conditions for `AST_CASE_SECTION` and `AST_DEFAULT_SECTION` alongside the existing `AST_LABEL_STATEMENT` check

### 2. Integration Tests
Create the following switch-based tests in `test/integration/` with both `-O0` and `-O1` flags:

1. **test_switch_break_o0**: The reduced state machine from CC99-014 with `-O0` flag (expected exit: 23)
2. **test_switch_break_o1**: The reduced state machine from CC99-014 with `-O1` flag (expected exit: 23)
3. **test_switch_consecutive_cases**: Multiple consecutive cases to verify case merging is preserved
4. **test_switch_default_after_break**: Default case appearing after break statements

### 3. Version Update
- Update `src/version.c` to stage 15600000

### 4. Validation
After fix, both `-O0` and `-O1` should produce exit code 23 for the state machine test.

## Implementation Steps

1. Read current `src/optimize.c` and locate the dead-code removal scan
2. Add the two new AST node type checks to the stopping condition
3. Create integration test files with appropriate cflags
4. Verify `-O0` and `-O1` both produce correct results
5. Update version to 15600000
