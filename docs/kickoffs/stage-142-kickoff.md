# Stage 142 Kickoff — Optimizer Infrastructure

## Summary of the Spec

Stage 142 introduces **AST-level optimizer scaffolding**: a new `src/optimize.c` / `include/optimize.h` module that proves the optimization infrastructure compiles, self-hosts, and slots cleanly into the compilation pipeline.

The optimizer is invoked after parsing and before code generation:

```
Source → Preprocessor → Lexer → Parser → AST → [Optimizer] → CodeGen → NASM
```

In this stage the pass performs **no transformations** — it is a pure tree-walking no-op. Subsequent stages (143+) will add constant-folding and dead-branch-elimination rules on top of this skeleton.

Two new command-line flags are wired end-to-end:
- `-O0` (default): optimization disabled, optimizer is a no-op.
- `-O1`: optimization enabled (in this stage still a no-op; future stages add rules).

The flags are recognized by both `ccompiler` and `bin/cc99`.

---

## Required Tokenizer Changes

**None.** No new tokens are needed.

---

## Required Parser Changes

**None.** No grammar or parser changes are required.

---

## Required AST Changes

**None.** No new AST node types are introduced. The optimizer walks the existing AST.

---

## Required Code Generation Changes

**None.** Code generation is unchanged. The optimizer runs *before* codegen and returns the AST (unmodified in this stage) to the caller.

---

## Test Plan

1. **Flag acceptance**: `ccompiler -O0` and `ccompiler -O1` compile without error.
2. **Output equivalence**: `-O0` and `-O1` produce identical NASM output on the same source (since the optimizer is a no-op).
3. **cc99 passthrough**: `bin/cc99 -O1 -o /tmp/hello /tmp/t.c` works end-to-end.
4. **Self-hosting**: `./build.sh --mode=self-host` passes (C0→C1→C2).
5. **Regression**: `./run_tests.sh` passes with all tests still working.

---

## Implementation Order

1. Create `include/optimize.h` with `optimize_translation_unit()` declaration.
2. Create `src/optimize.c` with no-op recursive `optimize_expr()` and `optimize_stmt()` helpers.
3. Update `CMakeLists.txt` to include `src/optimize.c` in the build.
4. Update `src/compiler.c`:
   - Add `#include "optimize.h"` at the top.
   - Add `int opt_level = 0;` variable in `main()`.
   - Add `-O0` and `-O1` flag parsing in `main()`.
   - Add `opt_level` parameter to `compile_one_file()`.
   - Call `optimize_translation_unit(ast, opt_level)` after parsing, before `print_ast` branch.
5. Update `bin/cc99` to recognize and forward `-O0` and `-O1` flags to `ccompiler`.
6. Update `src/version.c` to bump `VERSION_STAGE` to `"01420000"`.

---

## Key Implementation Details

### `src/optimize.c` Structure

- **`optimize_expr()`**: Generic recursive descent through all expression children (bottom-up). No case-based dispatch — all expression types are handled uniformly.
- **`optimize_stmt()`**: Switch-based dispatch on statement type (AST_BLOCK, AST_IF_STATEMENT, AST_WHILE_STATEMENT, AST_FOR_STATEMENT, etc.). Each case recursively visits condition, body, and initializer children, delegating expression optimization to `optimize_expr()`.
- **`optimize_translation_unit()`**: Entry point. If `opt_level == 0`, returns root unchanged. Otherwise, visits each top-level declaration; if it is an `AST_FUNCTION_DECL` with a body block, optimizes the function body.

### Return-Value Replacement Convention

Both helpers return `ASTNode *`. The return value must be used in the parent's child slot (child pointers are always updated). This design allows future stages to replace nodes (e.g., fold `AST_BINARY_OP` → `AST_INT_LITERAL`) by inserting transformation rules at the start of each case.

### Pipeline Integration

In `compile_one_file()`, the optimizer is called immediately after the parse-error guard and before the `print_ast` branch. This ensures the printed AST (when `--print-ast` is used) reflects any optimizations.

### Bootstrap Constraints

`src/optimize.c` must compile under the C0 compiler (the previous-stage binary):
- No VLAs, no C99 compound literals, no `//` comments.
- The `switch` in `optimize_stmt` uses only constant integer cases — valid.
- `for (int i = ...)` is valid C99 and already supported.

---

## Begin Implementation

Create `include/optimize.h` first.
