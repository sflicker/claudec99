# Stage 113 — Test Suite Reorganization

## Spec Summary

Reorganize four flat test suites into category subfolders to improve navigability and establish clear conventions for placing new tests. No compiler source changes, no test renaming, no new tests — purely a file layout refactor.

Affected suites:
- `test/valid/` — ~955 tests → 21 subfolders (arithmetic, bitwise, assignment, comparisons, casting, control_flow, functions, pointers, arrays, strings, chars, structs, unions, enums, typedefs, declarations, expressions, preprocessor, stdlib, floating_point, varargs, plus misc)
- `test/invalid/` — ~256 tests → legacy/ + 8 category subfolders
- `test/print_ast/` — 50 tests → legacy/ + constructs/
- `test/print_tokens/` — 100 tests → tokens/ + programs/
- `test/print_asm/` — 21 tests → stays flat

## Required Tokenizer Changes

None.

## Required Parser Changes

None.

## Required AST Changes

None.

## Required Code Generation Changes

None.

## Test Plan

### Phase 1: Runner Scripts (before moving files)
Update all 5 runner scripts (`test/valid/run_tests.sh`, `test/invalid/run_tests.sh`, `test/print_ast/run_tests.sh`, `test/print_tokens/run_tests.sh`, `test/print_asm/run_tests.sh`) to:
- Replace flat glob enumeration with `find`-based recursion
- Update companion file lookups (`.expected`, `.cflags`, `.args`) to use `$(dirname "$src")` instead of `$SCRIPT_DIR`

Run all suites with the updated scripts (while files are still in flat structure) to confirm find-based discovery works correctly.

### Phase 2: File Moves
Create subfolders and move test files in order:
1. `test/valid/` — 21 subfolders + misc
2. `test/invalid/` — legacy/ + 8 categories
3. `test/print_ast/` — legacy/ + constructs/
4. `test/print_tokens/` — tokens/ + programs/

Run each suite's tests after moving to catch placement errors early.

### Phase 3: Verification
Run full test suite (`./build.sh` or equivalent) and confirm pass/fail counts match pre-reorganization baseline.

### Phase 4: Documentation
Update README.md Testing section to document the new subfolder structure and placement rules for new tests.

## Implementation Order

1. Update `test/valid/run_tests.sh`
2. Update `test/invalid/run_tests.sh`
3. Update `test/print_ast/run_tests.sh`
4. Update `test/print_tokens/run_tests.sh`
5. Update `test/print_asm/run_tests.sh`
6. Run all suites (files still flat) — confirm all pass
7. Create and populate `test/valid/` subfolders (21 + misc)
8. Run `test/valid/` suite — confirm pass count matches
9. Create and populate `test/invalid/` subfolders (legacy + 8)
10. Run `test/invalid/` suite — confirm pass count matches
11. Create and populate `test/print_ast/` subfolders (legacy + constructs)
12. Run `test/print_ast/` suite — confirm pass count matches
13. Create and populate `test/print_tokens/` subfolders (tokens + programs)
14. Run `test/print_tokens/` suite — confirm pass count matches
15. Run full test suite — confirm all suites pass with same counts as before
16. Update README.md Testing section
17. Commit with no version bump

## Ambiguities & Decisions

**Ambiguity**: Spec says "No other changes to the valid runner" but `.libs` companion files also need to use `dirname($src)` since they move with their `.c` files. The valid runner uses `$SCRIPT_DIR/${name}.libs` for linker flags.

**Decision**: Apply the `dirname($src)` fix to the `.libs` lookup as well for consistency with other companion file handling. Spec intent is clear: all companion file lookups must be relative to the source file's location.

**Decision**: Place tests into subfolders using representative test name prefixes as the primary signal. For tests that are borderline between categories (e.g., `restrict_*` could go in declarations or expressions), follow the spec's representative prefix mapping.

**Decision**: The `misc/` subfolder catches tests that don't naturally fit any of the 21 main categories. This is a safety valve to avoid judgment calls on edge cases.

**No version bump**: `src/version.c` is unchanged (no compiler modifications).

## Scope

- In scope: runner script updates, test file moves, README.md Testing section update
- Out of scope: renaming tests, adding new tests, modifying integration/unit suites, compiler source changes
