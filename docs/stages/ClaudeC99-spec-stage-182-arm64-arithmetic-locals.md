# ClaudeC99 Stage 182 — ARM64 Integer Arithmetic and Local Variables (Phase 2)

## Goal

Extend the `arm64-macos` backend (`src/codegen_arm64.c`) from "a function body
that is one `return <literal>`" to straight-line integer code. That means
signed `int` and `long` local variables, arithmetic, bitwise and shift
operators, unary operators, assignment (including compound assignment, which
the parser already lowers), `++`/`--`, the comma operator, nested blocks, and
integer constants of any size.

This unlocks **ARM64 tier 2**: the 65 tests in `test/valid/arithmetic/`,
`test/valid/assignment/` and `test/valid/expressions/` that use only these
constructs. They are listed file by file in `test/valid/arm64_tiers.txt`. The
other tests in those directories need later phases and stay out of scope (see
"Tier 2 set").

The x86 targets (`x86_64-linux`, `x86_64-macos`) must not change.

---

## Scope

### Supported in this stage

| Category | Supported |
|----------|-----------|
| Types | signed `int` (32-bit), `long` / `long long` (64-bit) |
| Declarations | block-scope `int`/`long`/`long long` locals, with or without an initializer; multiple declarations per block; nested `{ }` blocks with shadowing |
| Statements | `return <expr>;`, expression statements, declarations, nested blocks; any number of statements per body |
| Primary | integer literals (any value representable in their lexer type), local variable references, parentheses |
| Binary | `+ - * / %`, `& | ^`, `<< >>` |
| Unary | `-`, `~`, unary `+` |
| Assignment | `=` to a local (compound `op=` arrives already lowered to `x = x op rhs` by the parser); the value of an assignment is the assigned value (chained assignment works) |
| Inc/dec | prefix and postfix `++` / `--` on locals |
| Comma | `a, b` evaluates left then right; the value is `b` |
| Functions | functions with no parameters, returning `int` or `long`; several such functions per translation unit (calls are Phase 4, so only `main` runs) |

### Still rejected (`arm64-macos: <construct> not yet supported`)

Everything else, including: comparisons and logical operators (`== != < <= >
>= && || !`), `?:`, `if`/loops/`switch`/`goto` (Phase 3); parameters and calls
(Phase 4); globals and string literals (Phase 5); pointers, arrays, structs,
unions, `&`, `*`, `sizeof` (Phase 6); `char`, `short`, `unsigned` types,
unsigned literals and casts (Phase 7); floating point (Phase 8); varargs (Phase
9).

Error messages follow the Stage 181 pattern: `<construct>` is the node's
readable name, or a more specific phrase where useful (`unsigned type`,
`char type`, `function parameter`, `comparison operator`, …).

The Stage 181 error `integer literal <v> out of range for this stage` is
**removed**, because large literals are now supported.

---

## Code generation design (`src/codegen_arm64.c`)

### Value model

- Every expression leaves its result in **`w0`** (int) or **`x0`** (long). An
  `int` result in `w0` has an undefined upper half of `x0` until it is
  explicitly sign-extended.
- An expression's type comes from a pure helper,
  `static int arm64_expr_is_long(CodeGenArm64 *cg, ASTNode *e)`:
  - integer literal: `decl_type` is `TYPE_LONG` / `TYPE_LONG_LONG`
  - variable: the variable's declared width
  - binary `+ - * / % & | ^`: long if either operand is long (usual
    arithmetic conversions, signed only)
  - `<< >>`: the type of the (promoted) **left** operand
  - unary: the operand's type
  - assignment / `++` / `--`: the type of the target variable
  - comma: the type of the right operand
- **Conversions:**
  - int → long: `sxtw x0, w0`
  - long → int (assignment to an `int`, or `return` from an `int` function):
    use `w0` (implicit truncation)

### Binary expressions — stack-machine style, as in the x86 backend

1. Evaluate the left operand, convert it to the operation type, then push:
   `str x0, [sp, #-16]!`
2. Evaluate the right operand and convert it (for shifts, the right operand
   is used as its own `w`/`x` width; only its low bits matter).
3. `mov x1, x0` then pop: `ldr x0, [sp], #16`
4. Apply the operation, using `w` registers for int and `x` registers for
   long:

| Op | Instruction(s) |
|----|----------------|
| `+` `-` `*` | `add` / `sub` / `mul` `r0, r0, r1` |
| `/` | `sdiv r0, r0, r1` |
| `%` | `sdiv r2, r0, r1` then `msub r0, r2, r1, r0` |
| `&` `\|` `^` | `and` / `orr` / `eor` |
| `<<` / `>>` | `lsl` / `asr` (signed right shift) |

Pushes stay 16-byte aligned because each push moves `sp` by 16.

### Unary operators

- `-`: `neg`
- `~`: `mvn`
- `+`: no code

### Integer constants: `emit_load_imm`

- **0..65535:** `mov w0|x0, #v`
- **Anything else:** `movz` with the low 16-bit chunk, then `movk … lsl #16/#32/#48`
  for each further non-zero chunk. That is 2 chunks for `w0`, up to 4 for `x0`.
- **Negative values** use the two's-complement bit pattern of the
  destination width.

(Literals in the AST are never negative, because unary minus is a separate
node. The helper still handles any 64-bit pattern, since `++`/`--` and later
phases reuse it.)

### Locals and frame

- **Pre-scan:** before the prologue, walk the function body and give **every**
  `AST_DECLARATION` its own slot. `int` takes 4 bytes at 4-byte alignment and
  `long` takes 8 bytes at 8-byte alignment. Slots are never reused across
  sibling blocks; that's simpler, and the waste is negligible.
- **Frame size:** round the total up to a multiple of 16.
  - Prologue: `stp x29, x30, [sp, #-16]!`, `mov x29, sp`, `sub sp, sp, #<frame>`
    (the `sub` is omitted when the frame is 0).
  - A frame larger than 4095 bytes is an error: `arm64-macos: stack frame
    larger than 4095 bytes not yet supported`.
- **Addressing:** `[x29, #-<offset>]`, using `ldr`/`str` with `w` or `x` by
  width. The assembler encodes negative offsets as `ldur`/`stur`.
- **Name lookup:** a scope stack. Entering a block pushes a scope; leaving pops
  it. Lookup searches innermost-first, so shadowing works.
- **`CodeGenArm64` gains:**
  - `Vec locals`: entries of `{ name, offset, is_long, scope_depth }`
  - the current function's name and return width
  - `frame_size`

  Use the existing `Vec` utility (`include/vec.h`). Free everything in
  `codegen_arm64_free`.

### Statements and returns

- **`return e;`**
  1. Evaluate `e` and convert it to the function's return width (sign-extend
     with `sxtw` if the function returns `long` and `e` is int).
  2. `b L<func>_epilogue`. Mach-O `L`-prefixed labels are assembler-local.
- **End of function:** `mov w0, #0` (reaching the closing `}` of `main`
  returns 0, per C99 5.1.2.2.3; other functions get the same harmless value),
  then:

  ```
  L<func>_epilogue:
  	mov	sp, x29
  	ldp	x29, x30, [sp], #16
  	ret
  ```

- **Expression statements:** evaluate and discard.
- **Declarations with initializers:** evaluate, convert to the declared width,
  store.
- **Declarations without initializers:** emit no code.

### Assignment and `++` / `--`

- **Assignment:**
  1. Evaluate the right-hand side and convert it to the target's width.
  2. Store it.
  3. The result stays in `w0`/`x0`.
- **Prefix `++x`:**
  1. Load `x`.
  2. `add` 1.
  3. Store.
  4. The result is the new value.
- **Postfix `x++`:**
  1. Load `x` into `r0`.
  2. `add r1, r0, #1`.
  3. Store `r1`.
  4. The result in `r0` is the old value.
- **Decrement:** the same, with `sub`.

---

## Part — Test infrastructure

- `test/valid/arm64_tiers.txt`:
  - Add the **65 tier-2 entries** (listed below), one `2 <path>` line each,
    under a `# Tier 2 — Phase 2 (stage 182)` comment.
  - Directories are not listed, because only part of each directory is in
    scope.
- No runner changes are expected. `ARM64_TIER` still defaults to `1`, so
  set `ARM64_TIER=2` to run tier 2.

---

## Tests

New target-neutral `test/valid` tests (they run on every target and join
tier 2):

1. **`arithmetic/test_large_literals__17.c`** exercises `movz`/`movk` for
   both widths:
   ```c
   int main(void) {
       long x = 4886718345L;   /* 0x123456789 */
       int  y = 305419896;     /* 0x12345678  */
       return (x - y) % 256;   /* 0x111111111 % 256 = 0x11 */
   }
   ```
2. **`arithmetic/test_long_to_int_truncate__42.c`** exercises long → int
   truncation on assignment:
   ```c
   int main(void) {
       long big = 4294967338L; /* 2^32 + 42 */
       int t = big;
       return t;
   }
   ```
3. **`expressions/test_nested_block_shadow__42.c`** exercises the scope
   stack and separate slots:
   ```c
   int main(void) {
       int x = 40;
       {
           int x = 1;
           x = x + 1;
       }
       return x + 2;
   }
   ```
4. **`arithmetic/test_main_falls_off_end__0.c`** exercises the implicit
   `return 0` from `main`:
   ```c
   int main(void) {
       int a = 5;
       a = a * 2;
   }
   ```

5. **Update `test/integration/test_arm64_skeleton_asm/`.** `big_literal.c` is
   now valid. Replace its error check with a positive one: compiling it
   succeeds, and the `.s` output contains `movz` and `movk`. Change the
   unsupported-construct case from a local variable to a comparison
   (`return 1 < 2;`), and expect `arm64-macos:` … `not yet supported`.

---

## Tier 2 set (65 files, from AST analysis of the three directories)

**`arithmetic/` (41):**

- `test_4_mod_2__0`
- `test_7_mod_5__2`
- `test_a_minus_b__6`
- `test_a_times_a_plus_1__12`
- `test_add__42`
- `test_add_a_b__42`
- `test_add_assign__42`
- `test_add_multiply__42`
- `test_declare_a__42`
- `test_divide__42`
- `test_int_div_long__5`
- `test_int_plus_long_literal__42`
- `test_multiply__42`
- `test_octal_permission__6`
- `test_octal_zero__0` (already tier 1)
- `test_paren__42`
- `test_postfix_dec__42`
- `test_postfix_dec_value__43`
- `test_postfix_in_expr__52`
- `test_postfix_inc__42`
- `test_postfix_inc_old_value__43`
- `test_postfix_inc_value__41`
- `test_prefix_dec__42`
- `test_prefix_dec_value__42`
- `test_prefix_in_expr__42`
- `test_prefix_inc__42`
- `test_prefix_inc_new_value__42`
- `test_prefix_inc_value__42`
- `test_rem_assign__2`
- `test_remainder_basic_one__1`
- `test_remainder_basic_two__2`
- `test_remainder_basic_zero__0`
- `test_remainder_left_assoc__0`
- `test_remainder_long__2`
- `test_remainder_precedence_div__2`
- `test_remainder_precedence_mul__1`
- `test_remainder_variables__1`
- `test_sub_assign__42`
- `test_sub_literals__42`
- `test_sub_with_vars__42`
- `test_unary_neg__214`

**`assignment/` (16):**

- `test_add_assign__42`
- `test_and_assign__2`
- `test_assignment_1__7`
- `test_assignment_2__7`
- `test_chained_assignment_1__42` through `test_chained_assignment_5__4`
- `test_div_assign__10`
- `test_mul_assign__42`
- `test_or_assign__7`
- `test_shl_assign__8`
- `test_shr_assign__4`
- `test_sub_assign__42`
- `test_xor_assign__5`

**`expressions/` (8):**

- `test_comma_assign_result__2`
- `test_comma_basic_result__2`
- `test_comma_chain__6`
- `test_comma_in_return__5`
- `test_comma_left_to_right__3`
- `test_comma_lower_than_assign__3`
- `test_const_expr__12`
- `test_variable_add__1`

Use the exact filenames (with `.c`) in the manifest. `test_octal_zero__0` is
already listed at tier 1 and needs no second entry, so the manifest gains 64
existing tier-2 files plus the 4 new tests: 68 entries.

The other 101 tests in these directories need later phases:

| Blocker | Count | Phase |
|---------|-------|-------|
| pointers, arrays, structs, `sizeof`, compound literals | ~62 | Phase 6 |
| comparisons, `?:`, `if`, `while` | ~25 | Phase 3 |
| function calls / parameters | 7 | Phase 4 |
| `char` / `short` / FP | ~7 | Phases 7–8 |

They are added to the manifest when those phases land.

---

## `src/version.c`

`VERSION_STAGE "01820000"`.

---

## Acceptance criteria

1. On macOS ARM64, `TARGET=arm64-macos ARM64_TIER=2 ./test/valid/run_tests.sh`
   reports **73 of N in scope** (5 tier-1 + 68 tier-2 entries; the overlap is
   counted once), with **73/73 passing natively**. `ARM64_TIER=1` still gives
   5/5.
2. **Default macOS suite** (`x86_64-macos`, Rosetta): all portable tests pass
   (2166 + 4 new valid = 2170).
3. **Linux-target `.asm`** for all test sources is byte-identical to Stage
   181, except the `__TIME__` test and the new tests.
4. **macOS self-host** (`./build.sh --mode=self-host`) passes C0→C1→C2. The
   expanded `src/codegen_arm64.c` must self-compile.
5. **Linux self-host** remains pending. Record the status.

## Out of scope

- Everything listed under "Still rejected"
- Register allocation or any optimization; ARM64 `-O2` stays a no-op
- `unsigned` arithmetic (`udiv`, `lsr`): Phase 7, together with type widths
- An ARM64 `print_asm` suite
