/* file_a.c: compiled together with file_b.c in a single ccompiler invocation.
 * Both files define a static helper named 'helper' to verify that per-file
 * parser/codegen tables are independent (no cross-file pollution). */

#include <stdio.h>

static int helper(int x) { return x * 2; }

int answer_a(void) { return helper(21); }
