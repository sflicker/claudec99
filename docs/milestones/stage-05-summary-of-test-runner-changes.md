All working. Here's a summary of the test runner changes:

test/valid/run_tests.sh — fixed PROJECT_DIR to go two levels up (../..) since tests moved from test/ to test/valid/.

test/valid/run_test.sh — same PROJECT_DIR fix.

test/invalid/run_tests.sh (new) — runs invalid test cases. For each test_*__<fragment>.c file:
- Compiles with the compiler (expects failure)
- Converts underscores in the fragment to spaces
- Checks stderr contains the expected error message (case-insensitive)
- Reports PASS/FAIL with summary

Ready to commit when you'd like.              