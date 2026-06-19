# ClaudeC99 stage 141 - system includes

## task
recently a test was made to use system includes (like /usr/include, /usr/include/x86_64-linux-gnu, etc) instead of the control headers in /test/include.
Stage 139 made numerous updates to enable this test to pass.

This task should add a flag to the cc99 wrapper that configures the compiler to use the system includes instead of the
control ones in /test/include.  Now this should only be available when running on linux with a x86_64 processor.

For the test suite. If being executed on linux with a x86_64 processor then the extra run_tests_sysinclude.sh integration
tests should also be executed. For test metrics the existing test suite counts could be in a portable totals section, and the system test totals could be in a separate one. 