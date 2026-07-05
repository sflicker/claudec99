# Milestone Summary

## Stage 175: Fix char *argv[] Parameter Array Decay - Complete

stage-175 fixes a parser bug where function parameters declared as a combination of pointer stars and array suffix (e.g., `char *argv[]`) were incorrectly adjusted to the wrong type after array decay. The fix reorders the branch chain in `parse_parameter_declaration()` so array decay is computed on top of declared pointer levels, not instead of them.

- Tokenizer: None.
- Parser: `parse_parameter_declaration()` reordered to check `d.is_array` before `d.pointer_count > 0`. When an array suffix exists, one extra `type_pointer()` level is applied on top of declared stars, correctly implementing C99 6.7.5.3p7 array-to-pointer decay. Pure arrays (`int a[3]`, zero declared stars) produce unchanged results.
- AST/Semantics: None; purely internal type-construction fix.
- Codegen: None.
- Tests: One new portable integration test `test_array_param_pointer_arg` demonstrates the fix by subscripting a `char *argv[]` parameter and passing the result to a function expecting `char *`, confirming both correct type-checking and runtime pointer arithmetic.
- Docs: Version bumped to `01750000` in `src/version.c`. New Stage 175 section added to `docs/outlines/checklist.md`. `docs/self-compilation-report.md` already includes stage-175 entry.
- Notes: The original spec's `getaddrinfo`/system-header repro was not ported to a portable test since it requires system headers; the new minimal test isolates the parser defect without system-header dependencies. Design decision: minimal portable test preferred over exact spec repro for better test suite portability.

## Final Results

Full test suite: 2075 portable + 192 system-include + 2 optional-library + 5 build-tool = 2274 total, all pass. Self-host C0→C1→C2 bootstrap completed with zero source changes and all tests passing at every stage.
