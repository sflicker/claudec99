# ClaudeC99 Stage 156 - correctness regression with `-O1` flag.

This report covers a correctness regression observed with cc99 `-O1`:

```text
/home/scott/code/claude/claudec99/bin/cc99
```

The reduced example was compared against GCC using
`gcc -std=c99 -O0 -Wall -Wextra -pedantic` and against cc99 `-O0`. GCC and
cc99 `-O0` return the same value, while cc99 `-O1` returns a different value.

## Summary

| ID | Area | Symptom | Priority |
|----|------|---------|----------|
| CC99-014 | O1 optimizer / switch-heavy state machine | `-O1` changes the result of enum-state `switch` loop code | High |

## CC99-014: O1 Miscompiles Switch-Based State Machine

### Original Benchmark Evidence

The benchmark runner was updated to compare cc99 `-O0` and `-O1` directly:

```sh
python3 tools/run_benchmarks.py --prod-compilers none --cc99-opt-levels O0 O1 --scale 5 --runs 5 --out-dir results/cc99-o1-retest --build-dir build/cc99-o1-retest
```

Most benchmarks had matching exit codes, but `09_state_machine` did not:

```text
cc99-O0: 232,232,232,232,232
cc99-O1: 41,41,41,41,41
```

Reducing the benchmark to a single generated stream pass still reproduced the
issue:

```text
GCC:      172
cc99 -O0: 172
cc99 -O1: 4
```

### Reduced Source

```c
enum State {
    ST_START = 0,
    ST_WORD = 1,
    ST_NUMBER = 2,
    ST_COMMENT = 3,
    ST_ERROR = 4
};

static char stream[9] = "a*7rh *4";

static int parse_stream(void) {
    enum State st = ST_START;
    int i;
    int score = 0;

    for (i = 0; stream[i]; i = i + 1) {
        char ch = stream[i];
        switch (st) {
        case ST_START:
            if (ch >= 'a' && ch <= 'z') {
                st = ST_WORD;
                score = score + 3;
            } else if (ch >= '0' && ch <= '9') {
                st = ST_NUMBER;
                score = score + 5;
            } else if (ch == '/') {
                st = ST_COMMENT;
                score = score + 7;
            }
            break;
        case ST_WORD:
            if (ch >= 'a' && ch <= 'z') {
                score = score + ch;
            } else {
                st = ST_START;
                score = score ^ i;
            }
            break;
        case ST_NUMBER:
            if (ch >= '0' && ch <= '9') {
                score = score * 3 + ch - '0';
            } else {
                st = ST_START;
                score = score + 11;
            }
            break;
        case ST_COMMENT:
            if (ch == '*') {
                st = ST_START;
                score = score + 13;
            } else if (ch >= '0' && ch <= '9') {
                st = ST_ERROR;
                score = score - ch;
            }
            break;
        default:
            score = score + ch;
            if ((score & 15) == 0) {
                st = ST_START;
            }
            break;
        }
    }

    return score + st;
}

int main(void) {
    return parse_stream() & 255;
}
```

Saved as:

```text
cc99_testing/test_o1_state_machine_switch.c
```

### Compile Commands

```sh
/home/scott/code/claude/claudec99/bin/cc99 -O0 -o /tmp/test_o1_state_machine_switch_o0 cc99_testing/test_o1_state_machine_switch.c
/home/scott/code/claude/claudec99/bin/cc99 -O1 -o /tmp/test_o1_state_machine_switch_o1 cc99_testing/test_o1_state_machine_switch.c
gcc -std=c99 -O0 -Wall -Wextra -pedantic -o /tmp/test_o1_state_machine_switch_gcc cc99_testing/test_o1_state_machine_switch.c
```

### Observed Behavior

```text
GCC:      23
cc99 -O0: 23
cc99 -O1: 4
```

cc99 `-O1` compiles successfully but changes the program result.

### Expected Behavior

Optimization must preserve the observable behavior of the program. cc99 `-O1`
should return `23`, matching GCC and cc99 `-O0`.

### Likely Fix Area

The reduced program combines:

- an enum state variable
- a loop over a NUL-terminated static character array
- a `switch` on the enum state
- assignments to the state variable inside switch cases
- `break` statements in each case
- a `default` case that may transition back to `ST_START`

The constant cc99 `-O1` result of `4` is suspicious because `4` is the numeric
value of `ST_ERROR`, suggesting the optimized code may be mishandling state
updates, switch dispatch, or the final `score + st` calculation.

Inspect O1 passes that rewrite or simplify:

- switch lowering or jump-table/branch generation
- enum-typed locals
- loop-carried local variables updated in switch cases
- control-flow joins after `break`
- dead-store or constant-propagation assumptions for `st` and `score`

### Regression Tests To Add

- The reduced `test_o1_state_machine_switch.c` under both `-O0` and `-O1`
- A switch loop where each case updates a loop-carried enum state
- A switch loop with a `default` case and case-local `break`
- Comparison of optimized and unoptimized exit codes for switch-heavy code
