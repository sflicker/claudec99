# Stage 131 Kickoff — `sizeof` Must Produce Unsigned `size_t`

## Overview

Stage 131 is a targeted correctness fix: `sizeof` must yield an unsigned
`size_t` result per C99 §6.5.3.4. The compiler currently treats `sizeof`
as signed `long`, causing wrong behavior in comparisons and arithmetic where
unsigned semantics matter.

## Spec Summary

The regression program (`test_sizeof_size_t__6.c`) returns 5 when it should
return 6 under a conforming C99 compiler:

| Check | Expression | Expected | Root cause of bug |
|-------|-----------|----------|-------------------|
| +1 | `sizeof(int) > -1` | false (0) | signed cmp: 4 > -1 = true → adds 1 incorrectly |
| +2 | `sizeof(char) - 2 > 0` | true (2) | signed sub: 1-2 = -1, -1 > 0 = false → misses 2 |
| +4 | `(sizeof(int) < 0) == 0` | true (4) | coincidentally correct even under signed |

The bug is that `sizeof` nodes set `node->is_unsigned = 0` (the default),
so the UAC (usual arithmetic conversion) logic in `uac_is_unsigned()` treats
the operands as signed and selects signed comparison/arithmetic instructions.

## Required Changes

### 1. Tokenizer Changes

None.

### 2. Parser Changes

None.

### 3. AST Changes

None.

### 4. Code Generation Changes (`src/codegen.c`)

Two additions, one per sizeof handler:

- **`AST_SIZEOF_TYPE` handler** (around line 3358): add `node->is_unsigned = 1;`
  after `node->result_type = TYPE_LONG;`
- **`AST_SIZEOF_EXPR` handler** (around line 3363): add `node->is_unsigned = 1;`
  at the top of the block (before the early-return branches), so all return
  paths inherit the unsigned flag.

The existing `uac_is_unsigned()` function already handles the case where one
operand is `TYPE_LONG` with `is_unsigned=1` and the other is `TYPE_INT` with
`is_unsigned=0` — it returns 1 (unsigned wins), selecting `setb`/`seta`/`sub`
instructions for unsigned arithmetic.

### 5. Version Bump (`src/version.c`)

Change `VERSION_STAGE` from `"01300000"` to `"13100000"`.

## Test Plan

- Add `test/valid/expressions/test_sizeof_size_t__6.c` — the exact regression
  program from the spec (three if-condition checks, expected exit code 6).
- Run full test suite to verify no regressions in the 34 existing sizeof tests.
- Self-host `./build.sh --mode=self-host`.

## Implementation Order

1. Code generation fix (2 lines in `src/codegen.c`)
2. Version bump (`src/version.c`)
3. New regression test
4. Full test suite
5. Self-host cycle
6. Update `docs/self-compilation-report.md`
7. Commit

## Notes & Decisions

- The fix is purely in `is_unsigned` — the `result_type` stays `TYPE_LONG`
  (same 8-byte width as `size_t` on SysV AMD64).
- The `expr_result_type()` function still returns `TYPE_LONG` for sizeof,
  which is correct for computing `common` in binary operators; the unsigned
  flag is read from `node->is_unsigned` after `codegen_expression` evaluates
  the child.
- The `eval_const_init` and `eval_const_expr` paths use `long` arithmetic;
  they are not updated here because the spec regression test only exercises
  runtime expression evaluation.
