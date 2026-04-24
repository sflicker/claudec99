# Stage-11-05-03 Kickoff: Conversions at the Function Boundary

## Spec Summary

Stage 11-05-03 completes the integer type system across function
boundaries by honoring declared parameter and return types at three
points:

1. **Argument conversion** — at each call site the argument value is
   converted to the declared parameter type (sign-extend when the
   source is narrower, truncate when wider, no-op when same size).
2. **Parameter storage** — at function entry each parameter is stored
   in a stack slot sized and aligned for its declared type (1/2/4/8
   bytes for char/short/int/long). The store width matches the
   declared type.
3. **Return-value conversion** — in `return expr;` the evaluated value
   is converted to the function's declared return type before being
   placed in the return register, using the same widen/truncate rules
   as assignment.

No grammar changes and no new AST node kinds are required. All
existing type-system mechanisms (sizes, promotions, load/store widths,
common arithmetic type) are reused.

## Delta From Previous Stage (11-05-02)

Stage 11-05-02 stamped the callee's declared return type onto the
`AST_FUNCTION_CALL` node's `decl_type` so downstream expression typing
sees the right type. It did **not** emit any conversion instructions
and it left parameter storage hard-coded to 4 bytes per parameter.
This stage emits the actual widen/truncate instructions at call sites
and at return statements, and stores parameters using their declared
type width and alignment.

## Spec Issues Noted (cosmetic, proceeding)

- Typo: `**Parameter storage using declared typse**` → `types`.
- Typo: `Argument=-to-parameter` → `Argument-to-parameter`.
- The return-widening example under Requirement 3 is malformed
  (`long f(x) { char x = 5; return 5; }` — missing parameter type and
  illegally redeclares `x`). Semantic intent is unambiguous.
- The Mixed-parameter-types example is missing its closing brace and
  code fence. Expected exit (13) is clear.

None of these block implementation.

## Planned Changes

### Tokenizer
- No changes.

### Parser
- No changes. (FuncSig already carries return and parameter types
  after 11-05-02.)

### AST
- No new node kinds. The existing `ASTNode.decl_type` (already set on
  `AST_FUNCTION_CALL` and `AST_FUNCTION_DECL`) is sufficient. No
  struct field additions are required.

### Code Generator
- Add a lightweight per-translation-unit function-signature table in
  `CodeGen` (collected from `AST_FUNCTION_DECL` nodes before any body
  is emitted) so call sites can look up parameter types by callee
  name.
- Track the current function's declared return type
  (`cg->current_return_type`) for use in `AST_RETURN_STATEMENT`.
- **At each `AST_FUNCTION_CALL`**: after evaluating argument `i` into
  `rax`/`eax`, emit a conversion from the argument's `result_type` to
  the callee's declared parameter type `i`:
  - source size < destination size → sign-extend (`movsx`/`movsxd`).
  - source size > destination size → truncate (value already in the
    low bits of `rax`; no explicit instruction needed since only the
    low bits of the argument register are read by the callee).
- **Function prologue**: size each parameter slot by its declared
  type (1/2/4/8 bytes) via `codegen_add_var(name, size)` and store
  from the argument register using a width-matched sub-register:
  char → `dil`/`sil`/`dl`/`cl`/`r8b`/`r9b`, short → `di`/`si`/`dx`/`cx`/
  `r8w`/`r9w`, int → `edi`/…, long → `rdi`/… .
- **`AST_RETURN_STATEMENT`**: after `codegen_expression` emits the
  value, convert `result_type` → `current_return_type`:
  - return `long`, source int-promoted → `movsxd rax, eax`.
  - return `char`, any int/long source → `movsx eax, al`.
  - return `short`, any int/long source → `movsx eax, ax`.
  - return `int`, source `long` → no instruction (the low 32 bits in
    `eax` are the truncated value).
  - return same kind as source → no instruction.

### Tests
Add valid tests covering:
- Argument narrowing (`char f(char); long→char`, expect `1`).
- Argument widening (`long f(long); char→long`, expect `5`).
- Return widening (`long f() { char x = 6; return x; }`, expect `6`).
- Return narrowing (`char f() { long x = 258; return x; }`, expect
  `2` — low byte of 258).
- Mixed parameter types (`short add(short, char)`, expect `13`).
- Interaction with arithmetic (conversion threaded through a binop
  inside arguments or the return expression).

### Documentation
- No grammar changes → no `docs/grammar.md` update required.

### Commit
- Single commit at end of stage (standard for the project).
