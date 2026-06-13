# ClaudeC99 Stage 113 — Test Suite Reorganization

## Goal

Reorganize the flat test suites into category subfolders to make the test
tree navigable and enforce a clear convention for placing new tests.  The
runners remain at the top level of each suite and are modified to recurse
through the hierarchy.  No test files are renamed; they are only moved to
the correct subfolder.  Companion files (`.expected`, `.cflags`, `.args`)
move with their `.c` file.

---

## Background

The test suites have grown to more than 1,600 tests spread across four
flat directories.  With 955 valid tests and 256 invalid tests all dumped
into a single directory it is increasingly hard to find existing tests or
decide where to place new ones.  This stage reorganizes the four large
suites (`test/valid/`, `test/invalid/`, `test/print_ast/`,
`test/print_tokens/`) into topic subfolders without touching the compiler
source or the integration and unit suites.

`test/print_asm/` has only 21 tests and a single natural grouping
(`test_stage_*`); it stays flat.

---

## Task 1 — Subfolder layout

### `test/valid/` (~955 tests → 21 subfolders)

| Subfolder | Representative test name prefixes |
|-----------|----------------------------------|
| `arithmetic/` | `add`, `a_minus_b`, `a_times_a`, `multiply`, `divide`, `increment`, `decrement`, `negation`, `remainder` |
| `bitwise/` | `bitand_`, `bitor_`, `bitxor_`, `bit_`, `lshift_`, `rshift_`, `complement_`, `and_long_`, `and_long_short` |
| `assignment/` | `assignment_`, `assign_through_`, `add_assign`, `and_assign`, `mul_assign`, `sub_assign`, `div_assign`, `or_assign`, `xor_assign`, `shl_assign`, `shr_assign` |
| `comparisons/` | `cmp_`, `eq_`, `ne_`, `gt_`, `lt_`, `ge_`, `le_`, `bang_`, `and_and`, `or_or`, `relational_` |
| `casting/` | `cast_`, `implicit_`, `arg_narrow_`, `arg_widen_`, `arg_conversion_` |
| `control_flow/` | `if_`, `basic_if`, `for_`, `while_`, `do_while_`, `break_`, `continue_`, `goto_`, `switch_`, `case_`, `return_`, `label_` |
| `functions/` | `func_`, `call_`, `param_`, `recurse_`, `inline_` |
| `pointers/` | `ptr_`, `pointer_`, `deref_`, `addr_of_`, `address_`, `null_ptr`, `void_ptr`, `assign_through_pointer` |
| `arrays/` | `array_`, `multi_dim_`, `subscript_` |
| `strings/` | `string_`, `adjacent_string_` |
| `chars/` | `char_literal_`, `char_`, `escape_` |
| `structs/` | `struct_`, `anon_struct_` |
| `unions/` | `union_`, `anon_union_` |
| `enums/` | `enum_` |
| `typedefs/` | `typedef_` |
| `declarations/` | `decl_`, `block_static_`, `file_scope_`, `const_`, `restrict_`, `short_`, `long_`, `signed_`, `unsigned_` |
| `expressions/` | `compound_literal_`, `designated_init_`, `sizeof_`, `comma_`, `ternary_`, `conditional_` |
| `preprocessor/` | `pp_`, `elifdef_`, `elifndef_`, `macro_`, `block_comment`, `line_comment` |
| `stdlib/` | `printf_`, `putchar_`, `puts_`, `stdlib_`, `assert_`, `string_hex_octal`, `memset_`, `memcpy_`, `strlen_` |
| `floating_point/` | `float_`, `double_`, `fp_` |
| `varargs/` | `va_`, `vararg_` |

> **Misc**: any test that does not naturally fit one of the above categories
> goes into `misc/`.

### `test/invalid/` (~256 tests → 2 subfolders)

`test/invalid/` contains two clearly distinct populations:

| Subfolder | Contents |
|-----------|----------|
| `legacy/` | The 149 numbered `test_invalid_NNN__*.c` tests |
| (new named tests) | All remaining descriptively-named tests stay in new category subfolders: |
| `aggregates/` | `anon_struct_*`, `anon_union_*`, `struct_*`, `union_*` |
| `declarations/` | `block_static_*`, `file_scope_*`, `long_long_*`, `short_long_*`, `signed_unsigned_*`, `unsigned_long_*`, `unsigned_signed_*`, `unsigned_struct` |
| `types/` | `bool_*`, `void_var_decl`, `void_global_var`, `void_signed`, `void_empty_return_*` |
| `const/` | `const_*`, `postfix_inc_const`, `postfix_inc_*` |
| `pointers/` | `arrow_*`, `dot_*`, `ptr_*`, `void_ptr_*`, `chained_missing_*`, `subscript_non_array_*` |
| `functions/` | `builtin_va_*`, `func_ptr_*`, `va_arg_*`, `void_fn_*` |
| `expressions/` | `compound_literal_*`, `conditional_*`, `designated_init_*`, `switch_case_*`, `local_array_too_many_*`, `multidim_*` |
| `control_flow/` | `for_decl_*` |
| `preprocessor/` | `pp_*` |

### `test/print_ast/` (50 tests → 2 subfolders)

| Subfolder | Contents |
|-----------|----------|
| `legacy/` | The 17 `test_stage_NN_*` tests (numbered stage regression tests) |
| `constructs/` | The remaining 33 descriptively-named tests covering expressions, statements, declarations, functions |

> The 33 named tests cover too many overlapping concerns (a `test_for_loop`
> and `test_while_loop` belong equally to "statements" and "control flow")
> to split further without creating tiny folders; a single `constructs/`
> subfolder is the right balance.

### `test/print_tokens/` (100 tests → 2 subfolders)

| Subfolder | Contents |
|-----------|----------|
| `tokens/` | The `test_token_*` tests (one test per token type) |
| `programs/` | The `test_program_*` tests (token-printing whole programs) |

### `test/print_asm/` (21 tests — stays flat)

All 21 tests follow `test_stage_*` naming with no clear sub-grouping.
The suite stays flat; no runner change required except the find-based
discovery described below (applied uniformly to all suites for
consistency).

---

## Task 2 — Runner script changes

Each runner script uses a flat glob to enumerate `.c` files.  Replace the
glob with a `find`-based recursive search and update the companion-file
lookup to be relative to the source file's directory rather than
`$SCRIPT_DIR`.

### `test/valid/run_tests.sh`

**Enumeration** — replace:

```bash
# Before
for src in "$SCRIPT_DIR"/*.c; do
```

with:

```bash
# After
for src in $(find "$SCRIPT_DIR" -name '*.c' | sort); do
```

**Companion `.expected` lookup** — replace:

```bash
# Before
expected_file="$SCRIPT_DIR/${name}.expected"
```

with:

```bash
# After
expected_file="$(dirname "$src")/${name}.expected"
```

No other changes to the valid runner.

### `test/invalid/run_tests.sh`

**Enumeration** — replace:

```bash
# Before
for src in "$SCRIPT_DIR"/test_*.c; do
```

with:

```bash
# After
for src in $(find "$SCRIPT_DIR" -name 'test_*.c' | sort); do
```

No companion files in the invalid suite; no other changes needed.

### `test/print_ast/run_tests.sh`

**Enumeration** — replace:

```bash
# Before
for src in "$SCRIPT_DIR"/test_*.c; do
```

with:

```bash
# After
for src in $(find "$SCRIPT_DIR" -name 'test_*.c' | sort); do
```

**Companion `.expected` lookup** — replace:

```bash
# Before
expected_file="$SCRIPT_DIR/${name}.expected"
```

with:

```bash
# After
expected_file="$(dirname "$src")/${name}.expected"
```

### `test/print_tokens/run_tests.sh`

Same two changes as `test/print_ast/run_tests.sh` (enumeration glob →
`find`; `$SCRIPT_DIR/${name}.expected` → `$(dirname "$src")/${name}.expected`).

### `test/print_asm/run_tests.sh`

Same two changes (enumeration glob → `find`; companion `.expected` lookup
→ `dirname "$src"`).

The `print_asm` runner uses `cd "$WORK_DIR"` before invoking the compiler
so the generated `.asm` lands in the work directory.  The `$src` argument
is an absolute path from `find "$SCRIPT_DIR" ...`, so the compile command
continues to work correctly after the `cd`.

---

## Task 3 — README.md test section update

The `README.md` contains a prose description of the test suites under the
**Testing** section.  Update it to:

1. Document the new subfolder structure for each suite.
2. Explain where new tests should be placed:
   - New `test/valid/` tests go in the matching category subfolder
     (`arithmetic/`, `control_flow/`, `floating_point/`, etc.).
   - New `test/invalid/` tests go in the appropriate category subfolder
     (or `legacy/` for backward-compat numbered tests).
   - New `test/print_ast/` or `test/print_tokens/` tests go in
     `constructs/` (print_ast) or `tokens/`/`programs/` (print_tokens).
   - The companion `.expected` file must be placed in the **same
     subfolder** as the `.c` file — the runner looks for
     `$(dirname "$src")/${name}.expected`.

Do not change the suite-count numbers or the runner invocation examples in
README.md; update only the structural description.

---

## Implementation order

1. Update `test/valid/run_tests.sh` (Task 2 changes).
2. Update `test/invalid/run_tests.sh` (Task 2 changes).
3. Update `test/print_ast/run_tests.sh` (Task 2 changes).
4. Update `test/print_tokens/run_tests.sh` (Task 2 changes).
5. Update `test/print_asm/run_tests.sh` (Task 2 changes).
6. Run all suites flat (no files moved yet) — all must still pass,
   confirming the find-based runners work correctly with the current layout.
7. Create the `test/valid/` subfolders and move files.  Run `test/valid/`
   runner after each batch of moves to catch mistakes early.
8. Create the `test/invalid/` subfolders and move files.  Run runner.
9. Create the `test/print_ast/` subfolders and move files.  Run runner.
10. Create the `test/print_tokens/` subfolders and move files.  Run runner.
11. Run the full test suite (`./build.sh` or equivalent) — all suites must
    still pass with the same counts as before.
12. Update `README.md` (Task 3).
13. Commit.

> **No version bump**: this stage makes no changes to the compiler source.
> `src/version.c` is left unchanged.

---

## Out of scope — do NOT do these in this stage

- **Renaming any test file** — file names must not change.
- **Adding new tests** — the test count stays the same; only the layout
  changes.
- **Modifying the integration or unit suites** — `test/integration/` and
  `test/unit/` have their own structure and are not touched.
- **Modifying `test/print_asm/`'s flat layout** — it stays flat.
- **Compiler source changes** — none.
- **Version bump** — no language changes, no version change.

---

## Tests

This stage adds no new test cases.  Success criterion: every suite reports
the same pass/fail counts before and after the reorganization.

---

## Docs

- **`README.md`** — update the Testing section to document the new
  subfolder structure and the placement rule for new tests (Task 3).
- No supplemental doc update is needed (no language features changed;
  parse-tree and status snapshots are unaffected).
- No `docs/self-compilation-report.md` update (no compiler change; the
  self-host cycle is not relevant to a pure file-move stage, though it
  should still pass if run).
