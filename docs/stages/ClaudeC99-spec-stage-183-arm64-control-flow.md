# ClaudeC99 Stage 183 — ARM64 Comparisons and Control Flow (Phase 3)

## Goal

Extend the `arm64-macos` backend (`src/codegen_arm64.c`) from straight-line
integer code (Stage 182) to integer code with full statement-level control
flow:

- comparison operators
- logical operators, including `!`
- the conditional operator `?:`
- `if`/`else`, `while`, `do`-`while` and `for` (with a C99 init declaration)
- `break` and `continue`
- `switch`/`case`/`default`
- `goto` and labels

This unlocks **ARM64 tier 3**, which has 366 new manifest entries:

- 163 existing tests that need Phase 3 constructs.
- 199 existing tests outside the Stage 182 directories that need only Phase 2
  constructs. They already pass natively today.
- 4 new tests.

Every entry is listed file by file under "Tier 3 set". The x86 targets
(`x86_64-linux`, `x86_64-macos`) must not change.

---

## Scope

### Supported in this stage (in addition to Stage 182)

| Category | Supported |
|----------|-----------|
| Comparison | `== != < <= > >=` on `int`/`long` operands (signed); the result is `int` 0 or 1 |
| Logical | `&&`, `\|\|` (short-circuit) and unary `!`; the result is `int` 0 or 1 |
| Conditional | `c ? a : b`; only the selected branch is evaluated; the result type is `long` if either branch is `long` |
| Selection | `if`, `if`/`else`, `switch` with `case`/`default`, including fallthrough, nested switches, and statements before the first label |
| Iteration | `while`, `do`-`while`, `for` (each clause optional; the init may be a declaration or declaration list whose scope is the loop) |
| Jumps | `break` (innermost loop or switch), `continue` (innermost loop, passing through any enclosing switch), `goto` and labeled statements (forward and backward, into and out of blocks) |

### Still rejected (`arm64-macos: <construct> not yet supported`)

- **Phase 4:** parameters and calls.
- **Phase 5:** globals, `static`/`extern` locals and string literals.
- **Phase 6:** pointers, arrays, structs, unions, `&`, `*`, `sizeof`, initializer
  lists and compound literals.
- **Phase 7:** `char`/`short`/`unsigned` types, unsigned and character literals,
  and casts.
- **Phase 8:** floating point.
- **Phase 9:** varargs.

The Stage 182 messages `comparison operator`, `logical operator` and
`logical not operator` are **removed**, because these operators are now
supported.

---

## Code generation design (`src/codegen_arm64.c`)

### Labels

- **Internal labels** are `L<n>_<kind>`, for example `L3_else`, `L7_loop_cond` or
  `L12_case`. `<n>` comes from a translation-unit-wide counter
  `CodeGenArm64.label_count`. Each construct takes one number and derives all its
  labels from it.
- **No collisions:** a C identifier cannot start with a digit, so these labels
  can never collide with the function-prefixed `L<func>_epilogue` from Stage 182.
- **User labels** are `L<func>_usr_<name>`.
- **Locality:** Mach-O treats any label starting with `L` as assembler-local.

### Width helper (`arm64_expr_is_long`) changes

- Comparisons, `&&`, `||` and `!` are `int`. Stage 182 returned the operand
  width for these nodes; that must change.
- Conditional: `long` if either the second or the third operand is `long`.

### Comparisons

1. Evaluate both operands at the common width (the usual arithmetic
   conversions), using the existing stack-machine sequence:
   ```
   str x0, [sp, #-16]!
   …
   mov x1, x0
   ldr x0, [sp], #16
   ```
2. `cmp r0, r1` (`w` or `x` by the common width).
3. `cset w0, <cc>`, where `<cc>` is `eq`, `ne`, `lt`, `le`, `gt` or `ge` (all signed).

### Logical operators

`a && b`:
```
	<a>
	cbz	rA, L<n>_false
	<b>
	cbz	rB, L<n>_false
	mov	w0, #1
	b	L<n>_end
L<n>_false:
	mov	w0, #0
L<n>_end:
```

`a || b` has the same shape, using `cbnz` to `L<n>_true`.

- **`rA`/`rB`:** `w0` or `x0`, by each operand's own width. The test uses the
  operand's full width, so a `long` with only upper bits set is true.
- **`!a`:** evaluate, `cmp rA, #0`, `cset w0, eq`.

### Conditional operator

```
	<c>
	cbz	rC, L<n>_else
	<a converted to the result width>
	b	L<n>_end
L<n>_else:
	<b converted to the result width>
L<n>_end:
```

Each branch is converted with `arm64_expr_as` before the join, so an `int`
branch of a `long` result is sign-extended (`sxtw`). The x86 backend currently
gets this wrong; see `docs/defects/ClaudeC99-defect-2-…`.

### Conditions in statements

`if`/`while`/`do`/`for` evaluate the condition and branch on zero:

- `cbz rC, <false label>` (`w0` or `x0` by width)
- `do`-`while` uses `cbnz rC, <body label>`

There is no fusion of comparisons into conditional branches (`b.lt`, …). That is
an optimization and is out of scope.

| Statement | Layout |
|-----------|--------|
| `if (c) T` | `<c>`; `cbz → L<n>_end`; T; `L<n>_end:` |
| `if (c) T else E` | `<c>`; `cbz → L<n>_else`; T; `b L<n>_end`; `L<n>_else:` E; `L<n>_end:` |
| `while (c) S` | `L<n>_loop_cond:` `<c>`; `cbz → L<n>_loop_end`; S; `b L<n>_loop_cond`; `L<n>_loop_end:` |
| `do S while (c)` | `L<n>_loop_body:` S; `L<n>_loop_cond:` `<c>`; `cbnz → L<n>_loop_body`; `L<n>_loop_end:` |
| `for (I; C; U) S` | push scope; I; `L<n>_loop_top:` [`<C>`; `cbz → L<n>_loop_end`]; S; `L<n>_loop_cond:` [U]; `b L<n>_loop_top`; `L<n>_loop_end:`; pop scope |

- **Jump targets:** `break` targets `L<n>_loop_end`. `continue` targets
  `L<n>_loop_cond`, which is the update for `for` and the condition for
  `while`/`do`.
- **`for` children:** the node always has 4 children, `[init, cond, update,
  body]`, and any of the first three may be NULL. A declaration (or
  `AST_DECL_LIST`) init goes through the declaration path; anything else is an
  expression.

### `break` / `continue`

- **Jump stack:** `CodeGenArm64` gains `Vec jumps` of
  `{ break_label, continue_label }`.
  - Loops push `{n, n}`.
  - A switch pushes `{n, -1}`.
- **`break`** uses the top entry.
- **`continue`** uses the innermost entry with `continue_label >= 0`.
- **Branches only:** both emit a single `b`, with no stack adjustment, because
  `sp` is the same at every statement boundary (see "Stack invariant").

### `switch`

1. **Collect the labels:** walk the body and collect every `AST_CASE_SECTION`
   and `AST_DEFAULT_SECTION`, assigning each a label number. Do not descend
   into nested `AST_SWITCH_STATEMENT` nodes. This is the same walk the x86
   backend does in `collect_switch_labels`. The labels go in a per-switch table
   on a switch stack.
2. **Dispatch from the register:** evaluate the controlling expression into
   `w0`/`x0` at its own width. For each case, in source order:
   - Get the case value from `strtol(case->children[0]->value, NULL, 0)`.
   - For an `int` discriminant, truncate it to 32 bits (C99 6.8.4.2p5: case
     values convert to the promoted controlling type).
   - If the value is in 0..4095: `cmp r0, #v`.
   - Otherwise: load it into `r1` with `arm64_emit_load_imm` and `cmp r0, r1`.
     This needs `arm64_emit_load_imm` to take a destination register number.
   - `b.eq L<m>_case`.
3. **Fallback:** `b` to the default label if there is one, else to
   `L<n>_switch_end`.
4. **Body:** emit it as ordinary statements. A case or default node emits its
   label, then its child statement if it has one.
5. **End:** `L<n>_switch_end:`. `break` targets it.

The discriminant is used only by the dispatch, which comes right after it is
evaluated. It is never spilled, so nothing is left on the stack across the
body. This differs from the x86 backend, which pushes it.

### `goto` and labels

- **Pre-walk:** collect every `AST_LABEL_STATEMENT` name in the function.
  - A duplicate is reported as `error: duplicate label '<name>' in function '<func>'`.
  - A `goto` to a name not in the table is reported as
    `error: undefined label '<name>' in function '<func>'`.
  - Both messages match the x86 backend.
- **Label:** emits `L<func>_usr_<name>:`, then its child statement.
- **`goto`:** emits `b L<func>_usr_<name>`.

Jumping into or out of blocks is safe, because frame slots exist for the whole
function and the scope stack is compile-time only.

### Frame pre-scan

The Stage 182 pre-scan descends only `AST_BLOCK` and `AST_DECL_LIST`. It now
descends **every non-NULL child** of every statement:

- if and else branches
- loop bodies
- `for` init
- case and default bodies
- labeled statements
- switch bodies

That way, declarations in those positions get slots.

- **Order:** slots are assigned in tree (child) order, which must equal the
  emitter's visiting order. Every statement emitter visits its children in
  child order and emits every declaration exactly once.
- **Expressions:** they never contain declarations, because compound literals
  are rejected.

### Stack invariant

At every statement boundary, `sp == x29 - frame_size`. Expression temporaries
are pushed and popped within one expression, and statements never sit inside
expressions. So `break`, `continue`, `goto`, case dispatch and `return` are
plain branches.

### `CodeGenArm64` gains

- `int label_count`, reset only in `codegen_arm64_init`
- `Vec jumps`: `{ int break_label; int continue_label; }`
- a switch stack: per switch, a `Vec` of `{ ASTNode *node; int label; }` plus
  `default_label`. Free each inner `Vec` when its switch ends.
- `Vec user_labels`: `const char *`, cleared per function

Free everything in `codegen_arm64_free`.

---

## Part — Test infrastructure

- `test/valid/arm64_tiers.txt`:
  - Add a `# Tier 3 — Phase 3 (stage 183)` section with the **366** entries
    below, one `3 <path>.c` line each.
  - Directories are still not listed.
- **Excluded on purpose** (never ARM64 entries): these test x86-only predefined
  macros.
  - `preprocessor/test_pp_predef_x86_64_ifdef__42.c`
  - `preprocessor/test_target_platform_macros__42.c`
- No runner changes. `ARM64_TIER` still defaults to `1`.
- **Rule for unexpected failures:** if a tier-3 candidate fails after
  implementation for a reason outside Phase 3 (a blocker the AST analysis
  missed), remove it from the manifest. List it, with its blocker, in the
  milestone. Do not widen the stage's scope to fix it.

---

## Tests

New target-neutral `test/valid` tests (they run on every target and join tier
3). The expected values were checked with Apple clang and with the current
x86_64-macos backend.

1. **`comparisons/test_cmp_long_neg_vs_int__1.c`**: a comparison at the common
   width (the `int` operand is sign-extended).
   ```c
   int main(void) {
       long a = -1;
       int  b = 0;
       return a < b;
   }
   ```
2. **`control_flow/test_switch_large_case_values__3.c`**: case values outside
   the `cmp` immediate range (negative, 4096, > 65535).
   ```c
   int main(void) {
       int x = 100000;
       int r = 0;
       switch (x) {
       case -5:     r = 1; break;
       case 4096:   r = 2; break;
       case 100000: r = 3; break;
       default:     r = 9;
       }
       return r;
   }
   ```
3. **`control_flow/test_continue_in_switch_in_loop__9.c`**: `continue` from
   inside a switch targets the enclosing loop.
   ```c
   int main(void) {
       int s = 0;
       for (int i = 0; i < 6; i++) {
           switch (i % 2) {
           case 0:  continue;
           default: break;
           }
           s += i;
       }
       return s;   /* 1 + 3 + 5 */
   }
   ```
4. **`declarations/test_many_locals_deep_slot__45.c`**: frame slots deeper than
   256 bytes (the Stage 182 `sub x9, x29, #off` path, untested until now).
   ```c
   int main(void) {
       long v0 = 0;
       long v1 = 1;
       /* … one declaration per line, through … */
       long v33 = 33;
       return v33 + v32 - v20;   /* 33 + 32 - 20; v33's slot is 272 bytes down */
   }
   ```
   The file spells out all 34 declarations, `v0` through `v33`, each set to its
   index.

**Deferred to the x86 defect fix** (see
`docs/defects/ClaudeC99-defect-2-x86-long-switch-case-and-mixed-width-conditional.md`).
These two tests fail on x86 today, so they are not added in this stage:

- `control_flow/test_switch_long_discriminant__7.c`
- `expressions/test_conditional_mixed_width__1.c`

The ARM64 design above handles both cases correctly.

---

## Tier 3 set (366 entries, from AST analysis of all of `test/valid`)

Each `test/valid` program was classified by the earliest roadmap phase whose
constructs cover it. The analysis read the program's `--print-ast` output,
checked declaration types, and checked the source for unsigned literal
suffixes.

- **Phase-2-only files** outside the Stage 182 manifest (199). They were run
  through the Stage 182 backend natively before this spec was written: all 199
  pass.
- **Phase-3 files** (163) are covered by this stage.
- **New tests** (4) are the ones listed above.

Use the exact filenames, with `.c` appended, in the manifest.

| Directory | Phase 2 (already pass) | Phase 3 | New |
|-----------|------------------------|---------|-----|
| arithmetic | — | 6 | — |
| bitwise | 28 | 5 | — |
| casting | — | 1 | — |
| comparisons | — | 33 | 1 |
| control_flow | 5 | 88 | 2 |
| declarations | 37 | 3 | 1 |
| enums | 13 | 1 | — |
| expressions | — | 17 | — |
| functions | 2 | — | — |
| misc | — | 2 | — |
| preprocessor | 110 | 2 | — |
| statements | — | 4 | — |
| stdlib | 2 | 1 | — |
| typedefs | 2 | — | — |
| **Total** | **199** | **163** | **4** |

### Phase-2-only files (199)

- **bitwise/:** test_bitand_basic__2, test_bitand_left_associative__1, test_bitand_var_int__2, test_bitor_basic__7, test_bitor_precedence_and__1, test_bitor_xor_and_chain__7, test_bitwise_and__0, test_bitwise_or__1, test_bitwise_xor__1, test_bitxor_basic__5, test_bitxor_precedence_and__0, test_complement_basic_neg1__255, test_complement_basic_neg3__253, test_complement_basic_zero__0, test_complement_nested__5, test_complement_precedence__1, test_complement_var_int__253, test_lshift_basic_1__8, test_lshift_basic_2__12, test_lshift_var_int__8, test_rshift_arith_negative__252, test_rshift_basic_1__4, test_rshift_basic_2__4, test_rshift_var_int__4, test_shift_left__16, test_shift_left_associative__4, test_shift_precedence_additive__24, test_shift_right__8
- **control_flow/:** test_block_scope__1, test_block_scope__2, test_nested_blocks_return__33, test_nested_var_scope__3, test_return_postfix_inc__10
- **declarations/:** test_const_func_param__0, test_const_int__3, test_decl_list_comma_assign__3, test_decl_list_comma_expr_both__6, test_decl_list_comma_expr_init__5, test_decl_list_comma_expr_second_init__5, test_decl_list_forward_ref__7, test_decl_list_init__12, test_decl_list_plain__7, test_decl_list_three_chain__22, test_decl_list_three_plain__42, test_decl_only_unused__42, test_declare_a__42, test_declare_only_a__0, test_hex_literal_basic__42, test_hex_literal_zero__0, test_limits_char_bit__8, test_limits_uint_max_defined__1, test_limits_ulong_max_defined__1, test_llong_max_defined__0, test_llong_min_defined__0, test_long_assign_from_int_sum__30, test_long_decl__42, test_long_literal_lowercase__42, test_long_literal_overflows_int__42, test_long_literal_suffix__42, test_long_minus_int__40, test_long_prefix_dec__41, test_long_times_int__50, test_long_unary_neg__42, test_multi_variables_1__5, test_multi_variables_2__10, test_outer_var_in_inner_scope__1, test_shadow_inner_scope__2, test_signed_alone__42, test_signed_int__42, test_ullong_max_defined__0
- **enums/:** test_enum_anonymous__2, test_enum_char_values__66, test_enum_const_expr_equality__0, test_enum_const_expr_logical_and__0, test_enum_const_expr_logical_or__0, test_enum_const_expr_paren__0, test_enum_const_expr_prior_const__0, test_enum_const_expr_relational__0, test_enum_const_expr_ternary__0, test_enum_explicit_values__11, test_enum_named_tag__2, test_enum_regression_basic__0, test_enum_trailing_comma__2
- **functions/:** test_func_ptr_parameter__0, test_multi_func_main_first__42
- **preprocessor/:** test_block_comment__42, test_elifdef_chain__0, test_elifdef_else__0, test_elifdef_nested__0, test_elifdef_taken__0, test_elifndef_false__0, test_elifndef_taken__0, test_line_comment__42, test_multiline_block_comment__42, test_null_directive__0, test_pp_define_compat_redef__42, test_pp_define_stmt_macros__2, test_pp_define_sum_expr__10, test_pp_define_value__42, test_pp_elif_after_ifndef_nonzero__42, test_pp_elif_after_ifndef_true__42, test_pp_elif_defined__42, test_pp_elif_defined_no_parens__0, test_pp_elif_defined_parens_multi__0, test_pp_elif_gt_ge_macro__42, test_pp_elif_logical_and_equality__42, test_pp_elif_macro_chain__42, test_pp_elif_multi_branch__2, test_pp_elif_paren_literal__42, test_pp_elif_skipped_after_ifdef__1, test_pp_error_in_if_0__42, test_pp_fn_macro_add__42, test_pp_fn_macro_expr_arg__6, test_pp_fn_macro_nested__6, test_pp_fn_macro_square__64, test_pp_fn_macro_zero_arg__42, test_pp_if_add_macros__42, test_pp_if_add_shift_precedence__42, test_pp_if_bitwise_and_equality__42, test_pp_if_bitwise_and_true__42, test_pp_if_bitwise_not__1, test_pp_if_bitwise_or_true__42, test_pp_if_bitwise_xor_false__1, test_pp_if_bitwise_xor_true__42, test_pp_if_defined_after_undef__1, test_pp_if_defined_logical_and__42, test_pp_if_defined_logical_or__42, test_pp_if_defined_no_parens__42, test_pp_if_defined_true__42, test_pp_if_defined_zero_false__42, test_pp_if_eq_macro__42, test_pp_if_false_branch__1, test_pp_if_ge_macro__42, test_pp_if_gt_macro__42, test_pp_if_le_macro__42, test_pp_if_logical_and_both_true__42, test_pp_if_logical_and_false__42, test_pp_if_logical_or_and_precedence__42, test_pp_if_logical_or_true__42, test_pp_if_long_unary_chain__42, test_pp_if_lt_macro__42, test_pp_if_macro_true__42, test_pp_if_mod_eq_zero__42, test_pp_if_mod_nonzero__1, test_pp_if_mul_precedence__42, test_pp_if_ne_macro__42, test_pp_if_nested_parens__42, test_pp_if_nonzero__42, test_pp_if_not_defined_paren__42, test_pp_if_paren_defined__42, test_pp_if_paren_literal__42, test_pp_if_paren_macro__42, test_pp_if_paren_unary_neg__1, test_pp_if_paren_unary_not__42, test_pp_if_shift_left__42, test_pp_if_shift_right_chain__42, test_pp_if_shift_right_zero__1, test_pp_if_true_branch__42, test_pp_if_unary_neg_literal__42, test_pp_if_unary_neg_macro_nonzero__42, test_pp_if_unary_neg_macro_zero__1, test_pp_if_unary_neg_zero__1, test_pp_if_unary_not_macro_false__42, test_pp_if_unary_not_macro_true__1, test_pp_if_unary_not_neg_one__1, test_pp_if_unary_not_one__1, test_pp_if_unary_not_zero__42, test_pp_if_unary_plus_literal__42, test_pp_if_unary_plus_macro_nonzero__42, test_pp_if_undef_then_if__42, test_pp_if_undefined_identifier__42, test_pp_ifdef_false_branch__1, test_pp_ifdef_macro_in_active_block__42, test_pp_ifdef_skip_invalid_source__42, test_pp_ifdef_true_branch__42, test_pp_ifndef_true_branch__42, test_pp_nested_ifdef_false_true__2, test_pp_nested_ifdef_true_false__1, test_pp_nested_ifdef_true_true__42, test_pp_predef_char_bit_if__42, test_pp_predef_claudec99_ifdef__42, test_pp_predef_linux_ifdef__42, test_pp_predef_lp64_if__42, test_pp_predef_sizeof_types_if__42, test_pp_predef_stdc_undef__1, test_pp_predef_stdc_version_if__42, test_pp_predef_unix_ifdef__42, test_pp_token_paste_id_num__42, test_pp_token_paste_join__42, test_pragma_operator_ignored__0, test_pragma_unknown_ignored__0, test_predefined_macro_line__0, test_predefined_macro_stdc_hosted__0, test_undef_basic__42, test_undef_use_before__42
- **stdlib/:** test_stdlib_exit_codes__0, test_stdlib_exit_failure__1
- **typedefs/:** test_typedef_65_aliases__3, test_typedef_block_scope__3

### Phase-3 files (163)

- **arithmetic/:** test_equals_with_vars__1, test_greater_equals_with_expr__1, test_less_than_with_vars__1, test_octal_basic__0, test_remainder_in_if__1, test_remainder_in_while__5
- **bitwise/:** test_bitand_precedence_equality__1, test_bitor_logical_and_mixed__1, test_bitwise_not__1, test_or_long_zero_int_zero__0, test_shift_precedence_relational__1
- **casting/:** test_uac_long_long_dominates_int__1
- **comparisons/:** test_bang_long_nonzero__0, test_bang_long_zero__1, test_chained_logical_and__0, test_chained_logical_and__1, test_chained_logical_and2__1, test_compound_equality_true__1, test_eq_false__0, test_eq_true__1, test_equals_with_negs__1, test_expr_eq_true__1, test_expr_lt_true__1, test_ge_false__0, test_ge_true__1, test_greater_equals_with_literals__1, test_greater_than_with_negs__1, test_gt_false__0, test_gt_true__1, test_int_ge_long__1, test_le_false__0, test_le_true__1, test_less_than_with_expr__0, test_logical_and__1, test_logical_and_false_false__0, test_logical_and_false_true__0, test_logical_and_true_false__0, test_logical_and_true_true__1, test_logical_or__1, test_lt_false__0, test_lt_true__1, test_ne_false__0, test_ne_true__1, test_unary_bang_nonzero__0, test_unary_bang_zero__1
- **control_flow/:** test_basic_for__55, test_basic_if__42, test_block_scope_shadowing__10, test_break__42, test_break_and_continue_in_while__9, test_break_in_do_while__42, test_break_in_for__10, test_break_in_for__42, test_break_in_while__42, test_break_nested__42, test_case_label_shift__0, test_continue_break__23, test_continue_in_do_while__42, test_continue_in_for__25, test_continue_in_for__42, test_continue_in_while__42, test_continue_nested__42, test_do_while__42, test_do_while_at_least_once__1, test_do_while_countup__42, test_do_while_long_cond__3, test_do_while_nested__42, test_else_if__42, test_for_basic__42, test_for_decl_basic__15, test_for_decl_body_sees_var__4, test_for_decl_continue__7, test_for_decl_multi__46, test_for_decl_post_sees_var__5, test_for_decl_shadow__42, test_for_empty_init__42, test_for_empty_update__42, test_for_long_cond__15, test_for_loop__55, test_for_loop_with_inc__55, test_for_loop2__55, test_for_loop3__55, test_for_loop4__55, test_for_nested__45, test_for_no_body_exec__0, test_for_sum__55, test_for_with_block__55, test_goto_backward_jump__3, test_goto_forward_jump__0, test_goto_inside_loop__4, test_if_4_mod_2__42, test_if_else_block__42, test_if_else_chain__42, test_if_else_if__42, test_if_with_blocks__42, test_if_with_else_block__42, test_if_x_false__9, test_if_x_true__5, test_label_with_block__7, test_loop_var_scope__42, test_nested_for_loop__20, test_nested_loops_with_break__6, test_nested_loops_with_continue__6, test_nested_while__5, test_sum_even__30, test_switch__42, test_switch_break__42, test_switch_break_in_loop_with_switch__42, test_switch_case_binary_add__42, test_switch_case_fallthrough_1__10, test_switch_case_fallthrough_2__10, test_switch_case_fallthrough_default__20, test_switch_case_relational__0, test_switch_case_unary_neg__42, test_switch_default_basic__42, test_switch_enum_basic__42, test_switch_explicit_enum_values__42, test_switch_fallthrough__42, test_switch_in_loop__42, test_switch_nested_20_deep__20, test_switch_return_in_default__42, test_switch_unlabeled_stmt_in_body__42, test_switch_with_case_1__42, test_switch_with_case_2__72, test_switch_with_case_default__99, test_switch_with_case_no_default__5, test_switch_with_return_inside_case__20, test_while_countdown__42, test_while_false_initially__42, test_while_long_counter__10, test_while_loop__42, test_while_nested__42, test_while_single_stmt__42
- **declarations/:** test_long_eq_long__1, test_long_if_cond__1, test_long_ne_int__1
- **enums/:** test_enum_const_expr_complement__0
- **expressions/:** test_basic_conditional_expression__42, test_basic_conditional_expression_else__24, test_comma_in_if__10, test_comma_in_while__6, test_conditional_basic_false__20, test_conditional_basic_true__10, test_conditional_comparison_cond__1, test_conditional_expression_with_nested__1, test_conditional_expression_with_nested2__1, test_conditional_expression_with_vars__42, test_conditional_expression_with_vars_else__24, test_conditional_false_only_evaluated__20, test_conditional_nested_true__3, test_conditional_prec_assignment__10, test_conditional_prec_logical_or__10, test_conditional_right_assoc__3, test_conditional_true_only_evaluated__10
- **misc/:** test_goto__42, test_multiple_returns__20
- **preprocessor/:** test_line_directive_file_and_line__0, test_line_directive_line_number__0
- **statements/:** test_switch_int_regression__5, test_switch_llong__7, test_switch_long_default__42, test_switch_long_small__3
- **stdlib/:** test_errno_enoent__1

### Still out of scope in the roadmap's tier-3 directories

| Blocker | Files | Phase |
|---------|-------|-------|
| calls / parameters | 6 `control_flow/` (`test_count_digits`, `test_gcd`, `test_is_prime`, `test_prime_count_lt_1000`, `test_short_circuit_and`, `test_switch_over_256_labels`) | Phase 4 |
| pointers / arrays / structs | 2 `control_flow/` | Phase 6 |
| `char` / unsigned | 3 `control_flow/`, 2 `statements/` | Phase 7 |

---

## `src/version.c`

`VERSION_STAGE "01830000"`.

---

## Acceptance criteria

1. **Native ARM64:** on macOS ARM64,
   `TARGET=arm64-macos ARM64_TIER=3 ./test/valid/run_tests.sh` reports
   **439 of 1297 in scope** (5 tier-1 + 68 tier-2 + 366 tier-3), with **439/439
   passing natively**. `ARM64_TIER=2` still gives 73/73, and `ARM64_TIER=1` still
   gives 5/5. Any manifest removal under the "unexpected failures" rule lowers
   these counts; record it in the milestone.
2. **Default macOS suite** (`x86_64-macos`, Rosetta): all portable tests pass
   (2170 + 4 new valid = 2174).
3. **Linux-target `.asm`:** byte-identical to Stage 182 for all test sources.
4. **macOS self-host** (`./build.sh --mode=self-host`) passes C0→C1→C2. The
   expanded `src/codegen_arm64.c` must self-compile.
5. **Linux self-host:** still pending. Record the status.

## Out of scope

- Everything listed under "Still rejected"
- Fusing comparisons into conditional branches, jump tables for `switch`, and any
  other optimization (ARM64 `-O2` stays a no-op)
- Unsigned comparisons (`lo`/`hi`/`ls`/`hs`): Phase 7
- Fixing the x86 bugs in defect 2 (a separate stage)
