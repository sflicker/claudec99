# ClaudeC99 Stage 184 — ARM64 Functions and Calls (Phase 4)

## Goal

Extend the `arm64-macos` backend (`src/codegen_arm64.c`) from single-function
integer code with control flow (Stage 183) to multi-function programs:

- function parameters (`int`/`long`), including parameters beyond the eighth
- direct calls to functions in the translation unit and to external functions
  (e.g. `labs`, `exit`)
- arguments converted to the callee's parameter types
- call results at the callee's return width
- `void` functions and `return;`
- block-scope function declarations

This unlocks **ARM64 tier 4**, which has 65 new manifest entries:

- 61 existing tests whose only remaining blocker is Phase 4.
- 4 new tests.

Every entry is listed file by file under "Tier 4 set". The x86 targets
(`x86_64-linux`, `x86_64-macos`) must not change.

---

## Scope

### Supported in this stage (in addition to Stage 183)

| Category | Supported |
|----------|-----------|
| Definitions | Any number of `int`/`long`/`long long` parameters (`const` allowed); `int`, `long`, `long long` or `void` return type; `static`, `extern` and `inline` functions |
| Calls | Direct calls by name (`AST_FUNCTION_CALL`) to a function defined in the translation unit or only declared (an external symbol); any number of arguments; calls nested in arguments, conditions, initializers and other expressions; recursion |
| Arguments | Converted to the callee's parameter type when the callee has a prototype; passed at their own width otherwise (no prototype, or no translation-unit declaration) |
| Returns | `return;` in any function; the call's value has the callee's return width |
| Declarations | Block-scope function declarations (they reach the backend as an empty `AST_TYPEDEF_DECL` statement) emit nothing |

### Still rejected (`arm64-macos: <construct> not yet supported`)

- **New in this stage:**
  - `call to variadic function` (Phase 9). This covers calls to a variadic
    function even if only fixed arguments are passed.
  - `<kind> parameter type`: a parameter that is not `int`/`long`/`long long`,
    in a definition or in the prototype of a called function. For example,
    `pointer parameter type` (Phase 6), `char parameter type` (Phase 7),
    `unsigned parameter type` (Phase 7), `floating-point parameter type`
    (Phase 8), `struct parameter type` (Phase 6).
  - `<kind> return type` at a call site, when the callee's return type (the
    call node's `decl_type`) is not `int`/`long`/`long long`/`void`.
- **Phase 5:** globals, `static`/`extern` locals and string literals.
- **Phase 6:** pointers, arrays, structs, unions, `&`, `*`, `sizeof`, initializer
  lists, compound literals, and **indirect calls** (`AST_INDIRECT_CALL`). An
  indirect call needs a pointer-typed callee value, so it moves from Phase 4 to
  Phase 6 (see "Roadmap changes").
- **Phase 7:** `char`/`short`/`unsigned` types, unsigned and character literals,
  and casts.
- **Phase 8:** floating point.
- **Phase 9:** varargs.

The Stage 182 messages `function parameter`, `function call`,
`void return type` and `return without a value` are **removed**, because these
constructs are now supported.

---

## Calling convention (Apple ARM64)

Apple's ARM64 ABI follows AAPCS64 with one difference that matters here:

- **Registers:** integer arguments 1–8 go in `x0`–`x7`. An `int` argument is
  in `w<n>`; the upper 32 bits of `x<n>` are unspecified, so the callee reads
  only `w<n>`.
- **Stack arguments (Apple difference):** arguments 9 and later are placed on
  the stack at their **natural size and alignment**, not one 8-byte slot each.
  An `int` takes 4 bytes at 4-byte alignment; a `long` takes 8 bytes at 8-byte
  alignment. The first stack argument is at offset 0 from the caller's `sp` at
  the `bl`.
  - Checked against Apple clang: for
    `f(int a, …, int h, int i, int j, long k, int l)` the callee reads `i` at
    `[sp]`, `j` at `[sp, #4]`, `k` at `[sp, #8]` and `l` at `[sp, #16]`.
- **Result:** in `w0` (`int`) or `x0` (`long`).
- **Alignment:** `sp` must be 16-byte aligned at every `bl`. The stack machine
  only moves `sp` by multiples of 16, so this always holds.
- **Callee-saved registers:** the backend uses only `x0`–`x2`, `x9` and `x10`
  (all caller-saved) besides `x29`/`x30`, and keeps every temporary on the
  stack across a call. Nothing else needs saving.

---

## Code generation design (`src/codegen_arm64.c`)

### Function lookup

- `CodeGenArm64` gains `ASTNode *tu`, set by `codegen_arm64_translation_unit`.
- `arm64_find_function(cg, name)` returns the **first** translation-unit-level
  `AST_FUNCTION_DECL` named `name`, whether a prototype or a definition. This
  mirrors `codegen_find_function_decl` in the x86 backend.
- The parser already rejects calls to unknown names (they become
  `AST_INDIRECT_CALL`) and wrong argument counts. The backend does not check
  them again.

### Parameters (callee side)

1. **Validation:** every `AST_PARAM` of the definition must be
   `int`/`long`/`long long` and not unsigned. Otherwise the definition is
   rejected with `<kind> parameter type`.
2. **Slots:** parameters get frame slots **before** the body's declarations,
   in parameter order, with the same `arm64_alloc_slot` sizes (int 4, long 8).
   The pre-scan does the same, so the two agree.
3. **Scope:** parameter locals are pushed at scope depth 0 after
   `vec_clear(&cg->locals)`. The body block is depth 1, so an inner-block
   declaration can shadow a parameter.
4. **Prologue:** after `sub sp, sp, #<frame>`:
   - parameter `i < 8`: store `w<i>`/`x<i>` to its slot
     (`arm64_slot_access(cg, "str", i, is_long, offset)`).
   - parameter `i >= 8`: `ldr w10/x10, [x29, #<16 + stack_offset>]`, then
     store `w10`/`x10` to its slot. `stack_offset` uses the Apple packing
     above, computed over the stack parameters in order. `16` skips the saved
     `x29`/`x30` pair.
5. Unnamed parameters cannot occur in a definition (the parser requires names);
   prototypes are never emitted.

### Calls (caller side)

For `f(a0, …, a(N-1))`, with `callee = arm64_find_function(cg, "f")`:

1. **Checks:**
   - `callee && callee->is_variadic`: reject with `call to variadic function`.
   - The call node's `decl_type` must be `int`/`long`/`long long`/`void`,
     otherwise reject with `<kind> return type`.
   - If `callee` has a prototype (`!callee->is_no_prototype`), each of its
     `AST_PARAM` children must be `int`/`long`/`long long` and not unsigned,
     otherwise reject with `<kind> parameter type`.
2. **Argument width:** for argument `i`, `want_long` is the parameter's width
   when the callee has a prototype and `i` is within its parameters.
   Otherwise it is the argument's own width (`arm64_expr_is_long`), which is
   what the default argument promotions give for `int`/`long`.
3. **Evaluate and push**, left to right: `arm64_expr_as(cg, a_i, want_long)`,
   then `str x0, [sp, #-16]!`. Argument `i` then sits at
   `[sp, #16 * (N - 1 - i)]`.
4. **Register arguments only (`N <= 8`):** pop in reverse,
   `ldr x<i>, [sp], #16` for `i = N - 1` down to `0`.
5. **Stack arguments (`N > 8`):**
   - Lay out arguments `8 … N-1` with the Apple packing: `off = align(off,
     size)`, then `off += size`. `A` is the total, rounded up to 16.
   - `sub sp, sp, #A`.
   - For each stack argument `i`: `ldr x0, [sp, #A + 16 * (N - 1 - i)]`, then
     `str w0/x0, [sp, #off_i]` at its width.
   - For `i = 0 … 7`: `ldr x<i>, [sp, #A + 16 * (N - 1 - i)]`.
   - After the call: `add sp, sp, #(A + 16 * N)`.
   - If `A + 16 * N > 4095`, reject with
     `call with more than 4095 bytes of arguments` (an immediate-range limit,
     as for frames).
6. **Call:** `bl _<name>`, for both local and external functions.
7. **Result:** in `w0`/`x0` at the callee's return width. A `void` call leaves
   nothing meaningful in `w0`; see "Void results" for the uses that are
   rejected.

Argument temporaries are pushed and popped within the call expression, so the
Stage 183 stack invariant (`sp == x29 - frame_size` at every statement
boundary) still holds.

### Width helper (`arm64_expr_is_long`)

- `AST_FUNCTION_CALL`: `arm64_kind_is_long(e->decl_type)` (the parser sets it
  to the callee's return type). A `void` call is not long.

### Statements and returns

- `AST_RETURN_STATEMENT` with no child: `b L<func>_epilogue`. With a child it
  is unchanged from Stage 182, apart from the "Void results" checks.
- `AST_TYPEDEF_DECL` as a statement (a block-scope typedef or a block-scope
  function declaration) emits nothing, as in the x86 backend.
- A `void` function that falls off its end still gets the `mov w0, #0` before
  the epilogue. It is harmless.

### Void results

The parser does not check these; the x86 backend does, in `codegen.c`. The
ARM64 backend reports the same messages, in the same places:

| Construct | Error |
|-----------|-------|
| `return expr;` in a `void` function | `error: void function '<func>' cannot return a value` |
| `return f();` where `f` returns `void` | `error: cannot use void function result as return value` |
| `x = f();` where `f` returns `void` | `error: cannot use void function result in assignment to '<x>'` |

Like the x86 backend, the ARM64 backend does not check other uses of a `void`
call's value (e.g. `f() + 1`).

### Functions

- The return type may now be `void`. A `void` function's `ret_is_long` is 0.
- `static` functions get no `.globl`, as in Stage 182. `inline` and `extern`
  definitions are emitted like any other definition.

### `CodeGenArm64` gains

- `ASTNode *tu`: the translation unit, for `arm64_find_function`.

Nothing new to free.

---

## Part — Test infrastructure

- `test/valid/arm64_tiers.txt`:
  - Add a `# Tier 4 — Phase 4 (stage 184)` section with the **65** entries
    below, one `4 <path>.c` line each.
  - Directories are still not listed.
- No runner changes. `ARM64_TIER` still defaults to `1`.
- **Rule for unexpected failures:** if a tier-4 candidate fails after
  implementation for a reason outside Phase 4 (a blocker the AST analysis
  missed), remove it from the manifest. List it, with its blocker, in the
  milestone. Do not widen the stage's scope to fix it.

---

## Tests

New target-neutral `test/valid/functions` tests (they run on every target and
join tier 4). The expected values were checked with Apple clang (native arm64)
and with the current x86_64-macos backend.

1. **`functions/test_ten_int_args__55.c`**: two arguments on the stack.
   ```c
   int sum10(int a, int b, int c, int d, int e,
             int f, int g, int h, int i, int j) {
       return a + b + c + d + e + f + g + h + i + j;
   }

   int main(void) {
       return sum10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
   }
   ```
2. **`functions/test_stack_args_mixed_width__49.c`**: Apple stack packing with
   mixed widths (`int` at +0 and +4, `long` at +8, `int` at +16).
   ```c
   /* Arguments 9..12 go on the stack.  Apple ARM64 packs them by natural
    * size and alignment: i at +0, j at +4, k at +8, l at +16. */
   long mix(int a, int b, int c, int d, int e, int f, int g, int h,
            int i, int j, long k, int l) {
       return i + j * 2 + k * 4 + l * 8;
   }

   int main(void) {
       return mix(0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3L, 4);
   }
   ```
3. **`functions/test_call_int_arg_to_long_param__1.c`**: an `int` argument to
   a `long` parameter is sign-extended. Without the `sxtw`, `x0` would hold
   `0x00000000FFFFFFFF` (positive) and the test would return 0.
   ```c
   /* An int argument to a long parameter is sign-extended. */
   long id(long x) {
       return x;
   }

   int main(void) {
       int a = -1;
       return id(a) < 0;
   }
   ```
4. **`functions/test_void_func_early_return__42.c`**: a `void` function, with
   `return;` and falling off the end. No existing tier-4 candidate defines a
   `void` function.
   ```c
   void check(int x) {
       if (x > 0)
           return;
       x = x + 1;
   }

   int main(void) {
       check(1);
       check(-1);
       return 42;
   }
   ```

---

## Tier 4 set (65 entries, from AST analysis of all of `test/valid`)

Every `test/valid` program not already in the manifest was classified by the
earliest roadmap phase whose constructs cover it. The analysis read the
program's `--print-ast` output (function definitions, their parameter and
return types, and every node of their bodies, with prototypes ignored),
checked top-level declarations, and checked the source for unsigned literal
suffixes. 64 programs need Phase 4 and nothing later. Three of them are left
out on purpose:

| File | Blocker | Phase |
|------|---------|-------|
| `functions/test_variadic_def_fixed_param__42` | call to a variadic function | 9 |
| `functions/test_variadic_def_two_fixed__3` | call to a variadic function | 9 |
| `stdlib/test_time_time__1` | `time(time_t *)`: pointer parameter type | 6 |

That leaves **61** existing tests, plus the **4** new ones.

| Directory | Existing | New |
|-----------|----------|-----|
| bitwise | 3 | — |
| control_flow | 6 | — |
| declarations | 4 | — |
| expressions | 1 | — |
| functions | 40 | 4 |
| misc | 1 | — |
| stdlib | 2 | — |
| typedefs | 4 | — |
| **Total** | **61** | **4** |

Use the exact filenames, with `.c` appended, in the manifest.

- **bitwise/:** test_complement_var_long__253, test_lshift_long_via_call__32, test_rshift_long_via_call__4
- **control_flow/:** test_count_digits__5, test_gcd__6, test_is_prime__1, test_prime_count_lt_1000__168, test_short_circuit_and__42, test_switch_over_256_labels__77
- **declarations/:** test_decl_before_def__42, test_decl_forward_call__42, test_long_long_return__42, test_multiple_decls__42
- **expressions/:** test_comma_as_function_arg__7
- **functions/:** test_block_scope_fn_decl_forward__0, test_call_in_binop__42, test_call_in_declaration_init__42, test_call_in_if_cond__42, test_call_nested__42, test_call_no_args__42, test_call_recursive_sum__45, test_call_returns_long__42, test_call_six_args__42, test_call_twice_sum__42, test_call_two_args__42, test_eight_arg_sum__42, test_extern_func_proto__6, test_extern_inline__0, test_fib_0__0, test_fib_1__1, test_fib_10__55, test_fib_2__1, test_forward_declaration__42, test_func_six_params__42, test_func_with_params__42, test_function_call__25, test_function_call_echo__25, test_function_call_part_of_expression__2, test_function_no_args__3, test_function_result_add__3, test_function_result_add__6, test_inline_func__0, test_param_shadowed_inner_scope__42, test_proto_unnamed_param__0, test_recursive_factorial__120, test_recursive_sum_3__6, test_recursive_sum_4__10, test_recursive_sum_up__15, test_seven_arg_call__42, test_simple_function_call__1, test_stack_arg_expr__42, test_sum_function__6, test_void_args__42, test_void_param_list__35
- **misc/:** test_max__88
- **stdlib/:** test_stdlib_abs__0, test_stdlib_exit__42
- **typedefs/:** test_typedef_enum_forward_ref__0, test_typedef_enum_fwd_across_fn__0, test_typedef_func_sig__5, test_typedef_long__5
- **New:** functions/test_ten_int_args__55, functions/test_stack_args_mixed_width__49, functions/test_call_int_arg_to_long_param__1, functions/test_void_func_early_return__42

The Stage 183 "still out of scope" calls/parameters row (6 `control_flow/`
files) is covered by this set.

### Still out of scope in `functions/`

95 of the 97 `functions/` programs are not yet in the manifest. This stage
takes 40 of them; the other 55 need a later phase:

| Earliest phase | Files | Typical blocker |
|----------------|-------|-----------------|
| 5 | 18 | globals, `static`/`extern` locals, string literals |
| 6 | 24 | pointers, arrays, structs, function pointers, indirect calls |
| 7 | 9 | `char`/`short`/`unsigned` parameters or values, casts |
| 8 | 2 | floating-point parameters |
| 9 | 2 | calls to variadic functions (the two variadic definitions above) |

---

## Roadmap changes

Update `docs/outlines/arm64-macos-backend-roadmap.md` with this spec:

- **Phase 4:** add a "Stage 184 scope" paragraph. Note that indirect calls move
  to Phase 6 (they need pointer-typed values) and that Apple packs stack
  arguments by natural size.
- **Phase 6:** add indirect calls (`blr`) to its goal.
- **Tier-to-category mapping:** row 4 becomes "Phase-4 files from all of
  `test/valid` (65 entries; see Stage 184 spec)".

---

## `src/version.c`

`VERSION_STAGE "01840000"`.

---

## Acceptance criteria

1. **Native ARM64:** on macOS ARM64,
   `TARGET=arm64-macos ARM64_TIER=4 ./test/valid/run_tests.sh` reports
   **504 of 1301 in scope** (5 tier-1 + 68 tier-2 + 366 tier-3 + 65 tier-4),
   with **504/504 passing natively**. `ARM64_TIER=3` still gives 439/439,
   `ARM64_TIER=2` 73/73 and `ARM64_TIER=1` 5/5. Any manifest removal under the
   "unexpected failures" rule lowers these counts; record it in the milestone.
2. **Default macOS suite** (`x86_64-macos`, Rosetta): all portable tests pass
   (2174 + 4 new valid = 2178).
3. **`test_arm64_skeleton_asm`** still passes. Its "not yet supported" probe is
   a `double` local (Phase 8), which this stage does not touch.
4. **Linux-target `.asm`:** byte-identical to Stage 183 for all test sources.
5. **macOS self-host** (`./build.sh --mode=self-host`) passes C0→C1→C2. The
   expanded `src/codegen_arm64.c` must self-compile.
6. **Linux self-host:** still pending. Record the status.

## Out of scope

- Everything listed under "Still rejected"
- Indirect calls and function pointers (Phase 6)
- Variadic calls, including Apple's all-variadic-arguments-on-the-stack rule
  (Phase 9)
- Extending narrow arguments (`char`/`short`) at call sites (Phase 7)
- Register allocation, leaf-function frame omission, tail calls and any other
  optimization (ARM64 `-O2` stays a no-op)
- Fixing the x86 bugs in defect 2 (a separate stage)
