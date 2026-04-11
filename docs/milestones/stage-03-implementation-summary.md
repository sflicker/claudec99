Stage 3 Complete

Results: 14 passed, 7 failed — every failure is a file with // comments (out-of-scope per your instruction). All in-scope tests pass:

- Equality: eq_true, eq_false, ne_true, ne_false
- Relational: lt_true, lt_false, gt_true, gt_false
- Stage-2 regressions: add, add_multiply, multiply, divide, paren, return

What shipped:
- Tokenizer: 6 new tokens (==, !=, <, <=, >, >=) with 2-char lookahead
- Parser: new equality → relational → additive precedence chain, all left-associative
- AST: unchanged — reuses AST_BINARY_OP with operator text in value
- Codegen: cmp ecx, eax + set<cc> al + movzx eax, al yielding 0/1; switched operator dispatch from value[0] to strcmp so multi-char ops disambiguate
- Test runners: run_test.sh and run_tests.sh now extract expected exit code from test_<name>__<expected>.c

❯ I have updated the test files to remove all comments.

Please rerun the tests and report:
- which tests pass
- which tests fail

If any tests fail, analyze why and propose minimal fixes consistent with the Stage 3 specification.

Do not introduce new features beyond the spec.

--- running -----
--- finished ----

● All 21 tests pass, 0 failures. No fixes needed — Stage 3 is complete and spec-compliant.
