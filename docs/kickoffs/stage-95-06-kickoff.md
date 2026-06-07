# Stage 95-06 Kickoff: Convert High-Risk Static Usages

## Spec Summary

Stage 95-06 converts four HIGH-priority fixed-capacity static arrays from the fixed-capacity-inventory.md to use the Vec dynamic module. Conversions are ordered by blast radius (lowest risk first) to minimize bootstrap failures:

1. **`PARSER_MAX_STRUCT_FIELDS`** (64) — Two local stack arrays in src/parser.c struct/union body parsing (lines ~394 and ~592). The overflow check uses hardcoded `64` instead of the constant, causing silent field loss when overflow occurs. Will be replaced with local `Vec tmp_fields_vec` in each function.

2. **`MAX_BREAK_DEPTH`** (32) — `CodeGen.break_stack[MAX_BREAK_DEPTH]` in include/codegen.h. Written at 4 sites without bounds check, silently corrupting adjacent CodeGen fields on overflow. Will be replaced with `Vec break_stack` in the CodeGen struct.

3. **`PARSER_MAX_TYPEDEFS`** (64) — `Parser.typedefs[PARSER_MAX_TYPEDEFS]` in include/parser.h. Used in parser_find_typedef (backward iteration), parser_register_typedef (append with check), and parser_leave_scope (in-place compaction). Will be replaced with `Vec typedefs`.

4. **`PARSER_MAX_FUNCTIONS`** (256) — `Parser.funcs[PARSER_MAX_FUNCTIONS]` in include/parser.h. Used by parser_find_function and parser_register_function. Will be replaced with `Vec funcs`.

## Tokenizer Changes

None.

## Parser Changes

- `include/parser.h`: Replace `FuncSig funcs[PARSER_MAX_FUNCTIONS]` with `Vec funcs` and `TypedefEntry typedefs[PARSER_MAX_TYPEDEFS]` with `Vec typedefs`
- `src/parser.c`: 
  - Update `parser_init` to initialize the new Vec fields
  - Update `parser_find_function` and `parser_register_function` to use Vec API
  - Update `parser_find_typedef` to use Vec API (handle backward iteration)
  - Update `parser_register_typedef` to use Vec API
  - Update `parser_leave_scope` to use Vec API (in-place compaction)
  - Update two struct/union body parsing sites (~lines 394 and 592) to use local `Vec tmp_fields_vec` instead of `StructField tmp_fields[PARSER_MAX_STRUCT_FIELDS]`

## AST Changes

None.

## Code Generation Changes

- `include/codegen.h`: Replace `BreakFrame break_stack[MAX_BREAK_DEPTH]` with `Vec break_stack`
- `src/codegen.c`:
  - Update `codegen_init` to initialize the new Vec field
  - Update all 4 break_stack write sites (while, do-while, for, switch) to bounds-check and use Vec API
  - Update all read sites to use Vec API

## Test Plan

- No new tests are needed; behavior is unchanged
- Run full test suite (1471 tests: 165 unit, 823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm) after each conversion
- Commit and update conversion checklist after each item passes all tests

## Bootstrap Compatibility Notes

The following patterns must be applied when converting to Vec (from fixed-capacity-inventory.md):

1. **Avoid inline pointer dereference in Vec retrieval**: Do not use `*(TypedefName *)vec_get(...)` inline; split to two statements:
   ```c
   T *p = (T *)vec_get(&v, i);
   result = *p;
   ```

2. **Use explicit size_t locals for overflow checks**: Ensure unsigned arithmetic by declaring local `size_t` variables rather than relying on cast-of-member arithmetic:
   ```c
   size_t lim = (size_t)-1;
   size_t cap = v->cap;
   if (cap > lim / 2)
       vec_fatal("capacity overflow");
   ```

3. **Error positions are unreliable during bootstrap failures**: If a bootstrap fails, binary-search by commenting out the second half of suspicious functions to isolate the construct causing the issue.

## Implementation Order

1. Convert `PARSER_MAX_STRUCT_FIELDS` — lowest blast radius (two isolated local variables)
2. Convert `MAX_BREAK_DEPTH` — medium blast radius (used at 4 write sites in codegen.c)
3. Convert `PARSER_MAX_TYPEDEFS` — medium-high blast radius (backward iteration, in-place compaction)
4. Convert `PARSER_MAX_FUNCTIONS` — highest blast radius (256 entries, used throughout parser)

After all conversions pass tests, update `docs/fixed-capacity-inventory.md` to mark the four items as DONE.

## Ambiguities

None. The spec clearly designates exactly four items as HIGH priority in the inventory.

## Decisions

- Conversions will be performed incrementally (one item at a time) to allow for rapid iteration and isolation of bootstrap failures
- All conversions will use the established Vec module (include/vec.h, src/vec.c)
- The hardcoded `64` check in struct field parsing will be replaced with a runtime Vec overflow check at the registration point
