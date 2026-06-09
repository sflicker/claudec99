/* file_b.c: compiled together with file_a.c in a single ccompiler invocation.
 * Defines the same 'helper' name as file_a.c — must compile cleanly with
 * fresh per-file state. */

#include <stdio.h>

static int helper(int x) { return x + 10; }

int answer_b(void) { return helper(32); }
