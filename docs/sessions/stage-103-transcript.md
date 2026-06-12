# stage-103 Transcript: Block-Scope Static Scalar Constant-Expression Initializers

## Summary

Stage 103 extends block-scope static scalar initializers to accept the full set of compile-time constant expressions — matching the expressiveness already supported for `case` labels (stage 77), enum values (stage 99), and file-scope globals (stage 100). Previously, only `AST_INT_LITERAL`, `AST_CHAR_LITERAL`, and negated literals were accepted; expressions like `3 * 4`, `1 << 12`, `0xF0 | 0x0F`, and `sizeof(long) * 8` were rejected with a compile error. This is a codegen-only stage with no parser, AST, or grammar changes. The implementation adds a recursive `eval_const_init` helper that mirrors the parser's `eval_const_expr` function but operates on the AST after parsing.

## Plan

1. Add `eval_const_init` helper in `src/codegen.c` before `codegen_statement`.
2. Replace the 3-case ad-hoc check in the SC_STATIC scalar arm with a call to `eval_const_init`.
3. Bump `VERSION_STAGE` to `01030000` in `src/version.c`.
4. Create new test cases covering arithmetic, bitwise, shift, sizeof, and error scenarios.
5. Run the full test suite and self-host verification.
6. Update documentation.

## Implementation Steps

### Step 1: Add `eval_const_init` helper

Added `eval_const_init(ASTNode *node, const char *varname)` static function in `src/codegen.c` (before `codegen_statement`, line ~4330) to recursively evaluate parsed AST initializer subtrees as compile-time integer constants.

The function handles:
- `AST_INT_LITERAL`: convert with `strtol(node->value, NULL, 10)` — **initial attempt used base 10 only**, but hex literals (e.g., `0xFF`) require base 0 to auto-detect the radix. Fixed during testing.
- `AST_CHAR_LITERAL`: extract first byte cast to `(long)(unsigned char)`.
- `AST_SIZEOF_TYPE`: two paths — if `node->full_type` is present, use `type_size(node->full_type)`; otherwise (for scalar sizeof), use `type_kind_bytes(node->type_kind)`. **Bug found during testing**: when sizeof is applied to a type, `full_type` is NULL but `type_kind` contains the scalar kind (e.g., `TYPE_KIND_INT`). The initial implementation only checked `full_type`, causing scalar sizeof to fail.
- `AST_UNARY_OP` (child_count == 1): recursively evaluate the child and apply the operator (`-`, `~`, `!`, `+`).
- `AST_BINARY_OP` (child_count == 2): recursively evaluate both children and apply the operator (`+`, `-`, `*`, `/`, `%`, `<<`, `>>`, `&`, `^`, `|`). Division and modulo include a div-by-zero guard that calls `compile_error` with context.
- All other node types: call `compile_error` with the variable name for context.

### Step 2: Replace ad-hoc check in `codegen_statement`

In the SC_STATIC scalar arm (lines ~4377–4399), replaced the 23-line sequence of three `if/else if/else` cases with a 7-line version:

```c
/* Scalar static: evaluate the initializer as a compile-time constant.
 * Stage 103: eval_const_init handles the full expression tree. */
long init_value = 0;
int is_initialized = 0;
if (node->child_count > 0) {
    init_value = eval_const_init(node->children[0], node->value);
    is_initialized = 1;
}
```

The old code had three branches (int literal, char literal, negated literal) and fell through to `compile_error` for anything else. The new code delegates all evaluation to `eval_const_init`, which recursively handles the full expression tree and reports precise error messages.

### Step 3: Version bump

Bumped `VERSION_STAGE` from `01020000` to `01030000` in `src/version.c`.

### Step 4: Create test cases

Added 9 new test cases:

**Valid tests (7):**
- `test_static_const_arith__0.c` — `static int x = 3 * 4 + 2; return x - 14;` (arithmetic expression)
- `test_static_const_shift__0.c` — `static int page = 1 << 12; return page - 4096;` (left shift)
- `test_static_const_bitor__0.c` — `static int mask = 0xF0 | 0x0F; return mask - 255;` (bitwise OR with hex)
- `test_static_const_complement__0.c` — `static int all = ~0; return (all & 0xFF) == 0xFF ? 0 : 1;` (unary complement)
- `test_static_const_sizeof__0.c` — `static long sz = sizeof(long) * 8; return sz - 64;` (sizeof in expression)
- `test_static_const_persist__0.c` — `static int step = 2 * 3; static int n = 0; n += step; return n;` (persistence across calls)
- `test_static_const_enum_expr__0.c` — `enum { BASE = 10 }; static int limit = BASE * 2 + 5; return limit - 25;` (enum in expression)

**Invalid tests (2):**
- `test_static_const_var_ref__error.c` — `static int x = n + 1;` where `n` is a parameter (variable reference, not constant)
- `test_static_const_call__error.c` — `static int x = g();` where `g()` is a function call (not a constant expression)

## Issues Found During Implementation

### Bug 1: Hex Literals Not Recognized in `strtol`

**Problem:** The initial implementation used `strtol(node->value, NULL, 10)` for `AST_INT_LITERAL` nodes, which only recognizes base-10 integers. Hex literals like `0xFF` parsed correctly by the lexer were rejected as non-constant.

**Fix:** Changed to `strtol(node->value, NULL, 0)`, which auto-detects the base from the prefix (`0x` for hex, `0` for octal, decimal otherwise).

### Bug 2: Scalar `sizeof` Missing Fallback

**Problem:** The initial implementation only checked `if (node->full_type)` for `AST_SIZEOF_TYPE`, but when sizeof is applied to scalar types (e.g., `sizeof(int)`), the parser stores the type information in `node->type_kind` (an enum value), not in `node->full_type`. Tests using `sizeof(long)` or similar scalar types failed with "must be a constant expression".

**Fix:** Added a fallback: `if (node->full_type) return (long)type_size(node->full_type); else if (node->type_kind >= 0) return (long)type_kind_bytes(node->type_kind);`. The second branch handles scalar sizeof by looking up the byte size from the type-kind enum.

## Tests Run and Results

Built with `cmake -S . -B build && cmake --build build`.

Ran full test suite: `./test/run_all_tests.sh`

**All 1569 tests pass:**
- Valid: 894 (up from 887 in stage 102)
- Invalid: 253 (up from 251 in stage 102)
- Integration: 86
- Print-AST: 50
- Print-tokens: 100
- Print-asm: 21
- Unit: 165

No regressions.

## Self-Host Results

Ran `./build.sh --mode=self-host`:
- C0 (GCC-built): all 1569 tests pass
- C1 (C0-compiled): all 1569 tests pass
- C2 (C1-compiled): all 1569 tests pass

No compiler source changes were needed. The compiler's own codebase uses block-scope static scalar initializers only with simple literals (0, 1, NULL, -1), all of which go through the `AST_INT_LITERAL` or `AST_CHAR_LITERAL` branches — unchanged in behavior from stage 102.

## Generated Artifacts

1. `docs/milestones/stage-103-milestone.md` — Stage 103 milestone summary
2. `docs/sessions/stage-103-transcript.md` — This session transcript
3. Updated `README.md` — "Through stage 103" line, extended static-variable bullet, refreshed test totals
