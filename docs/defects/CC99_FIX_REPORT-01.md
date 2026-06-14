# cc99 Fix Report From Benchmark Bring-Up

This report captures cc99 issues found while creating the benchmark suite in
`/home/scott/code/c/c_benchmarks`. The final benchmarks were adjusted to avoid
known cc99 failures so timing comparisons can run, but the original failures
should be reduced and fixed in the cc99 project.

Tested compiler:

```text
/home/scott/code/claude/claudec99/bin/cc99
```

## Summary

| ID | Area | Symptom | Priority |
|----|------|---------|----------|
| CC99-BENCH-001 | Parser / constant expressions | Array bounds reject `MACRO + constant` | High |
| CC99-BENCH-002 | Codegen / static aggregate storage | Valid static aggregate programs compile then segfault | High |
| CC99-BENCH-003 | Floating-point codegen | Deterministic double benchmark returns different checksum | Medium |

## CC99-BENCH-001: Array Bounds Reject Constant Expressions

### Symptom

cc99 rejects array bounds that use an integer constant expression involving a
macro and an addition operator.

### Repro

```c
#define LIMIT 65000

static char composite[LIMIT + 1];

int main(void) {
    composite[0] = 1;
    composite[LIMIT] = 2;
    return composite[0] + composite[LIMIT];
}
```

Compile:

```sh
/home/scott/code/claude/claudec99/bin/cc99 -o /tmp/repro_array_bound repro_array_bound.c
```

Observed error:

```text
error: expected ']', got '+' ('+')
```

### Expected Behavior

C99 permits integer constant expressions in array bounds. This should compile
and produce an executable returning `3`.

### Workaround Used In Benchmarks

The benchmark suite replaced expressions such as:

```c
static char composite[LIMIT + 1];
```

with literal sizes:

```c
static char composite[65001];
```

### Likely Fix Area

Array declarator parsing appears to accept only a narrower expression form than
the existing constant-expression evaluator supports. Reuse or extend the same
integer constant-expression path used for enum constants, case labels, and
file-scope integer initializers.

### Regression Tests To Add

- File-scope `static char a[N + 1];`
- Block-scope `int a[N + 1];` if fixed-size block arrays are intended to use
  the same path
- Multi-dimensional `int grid[W + 1][H + 2];`
- Typedef array bound: `typedef int Row[N + 1];`

## CC99-BENCH-002: Runtime Crashes With Static Aggregate Arrays

### Symptom

Several benchmark drafts compiled successfully with cc99 but crashed at runtime
with `SIGSEGV`. The common pattern was large file-scope aggregate storage:
global arrays of structs, structs containing pointers, structs containing
unions, or initialized two-dimensional character arrays.

The runner later classified these as `run-signaled` with exit code `-11`.

### Patterns That Triggered The Issue

#### Global Struct Array

```c
#define N 9000

struct Item {
    int key;
    int weight;
    int tag;
};

static struct Item items[N];

int main(void) {
    items[0].key = 1;
    items[N - 1].tag = 2;
    return items[0].key + items[N - 1].tag;
}
```

#### Global Struct Array With Pointers

```c
#define NODES 32000

struct Node {
    int value;
    struct Node *left;
    struct Node *right;
    struct Node *next;
};

static struct Node nodes[NODES];

int main(void) {
    nodes[0].value = 7;
    nodes[0].next = &nodes[1];
    nodes[1].value = 5;
    return nodes[0].next->value;
}
```

#### Global Struct Array Containing A Union

```c
#define N 24000

typedef struct Pair {
    int a;
    int b;
} Pair;

typedef union Payload {
    int i;
    Pair pair;
    char bytes[8];
} Payload;

typedef struct Record {
    int kind;
    Payload payload;
    int score;
} Record;

static Record records[N];

int main(void) {
    records[0].payload.i = 42;
    records[N - 1].score = 3;
    return records[0].payload.i + records[N - 1].score;
}
```

#### Initialized Two-Dimensional Character Array

```c
static char words[3][12] = {
    "alpha",
    "bravo",
    "charlie"
};

int main(void) {
    return words[0][0] + words[2][0];
}
```

### Expected Behavior

Each program should compile and run without a signal. The simple repros above
should return deterministic small integer values.

### Workaround Used In Benchmarks

The final benchmark suite replaced large global aggregate arrays with scalar
backing arrays and local aggregate values. For example, the struct-sort
benchmark now uses:

```c
static int keys[N];
static int weights[N];
static int tags[N];
```

instead of:

```c
static struct Item items[N];
```

### Likely Fix Area

This looks like a codegen or object-layout issue for file-scope aggregate
storage. Areas to inspect:

- Global aggregate size calculation
- Field offset calculation inside array elements
- BSS/data emission for large aggregate arrays
- Address calculation for `global_array[index].field`
- String literal initialization of `char[N][M]`
- Structs containing pointer fields and union fields

### Regression Tests To Add

- Small and large global arrays of structs
- Global arrays of structs with pointer fields
- Global arrays of structs containing unions
- Access to first, middle, and last elements
- Read/write through `arr[i].field`
- Initialized `char words[N][M]` from string literals

## CC99-BENCH-003: Floating-Point Checksum Mismatch

### Symptom

The deterministic floating-point benchmark compiles and runs under cc99, GCC,
and Clang, but cc99 returns a different checksum.

Current benchmark:

```text
benchmarks/06_float_nbody.c
```

From `results/benchmark-results.csv` after a longer run with
`BENCH_SCALE=100` and `--runs 7`:

| Compiler | Exit Codes |
|----------|------------|
| cc99 | `0,0,0,0,0,0,0` |
| gcc -O0 | `251,251,251,251,251,251,251` |
| gcc -O2 | `251,251,251,251,251,251,251` |
| clang -O0 | `251,251,251,251,251,251,251` |
| clang -O2 | `251,251,251,251,251,251,251` |

### Expected Behavior

For this benchmark, cc99 should match GCC/Clang at least at `-O0` semantics.
The program uses deterministic double arithmetic and does not call libm.

### Notes

The final return value comes from:

```c
out = (int)total;
return out & 255;
```

The mismatch may come from floating-point arithmetic, double-to-int conversion,
struct field load/store for doubles, comparison/control-flow behavior, or ABI
handling of double temporaries.

### Suggested Reduction Path

Create smaller tests around:

1. Double addition and multiplication in nested loops
2. Double values stored in and loaded from struct fields
3. Double-to-int casts after accumulated arithmetic
4. Repeated updates through `struct Body bodies[N]`
5. Mixed integer and double expressions

Start with a minimal version of `benchmarks/06_float_nbody.c` using 2 or 3
bodies and a small fixed step count, then compare the final integer checksum
against `gcc -O0`.

## Final Benchmark Status

After benchmark-side workarounds, the benchmark suite runs successfully across
cc99, GCC, and Clang.

Smoke command:

```sh
python3 tools/run_benchmarks.py --scale 1 --runs 1
```

Longer run observed in `results/benchmark-results.csv`:

```sh
python3 tools/run_benchmarks.py --scale 100 --runs 7
```

The longer run is useful for performance comparisons and also confirms the
floating-point checksum mismatch described above.
