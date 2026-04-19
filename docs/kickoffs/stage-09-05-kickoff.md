# stage-09-05 Kickoff: Function Declarations

## Spec Summary

Stage 9.5 introduces function declarations (prototypes). A declaration has
the same header as a definition — return type, identifier, parameter list —
but ends with `;` instead of a block. Declarations and definitions both
appear at file scope. A function may be called whenever either a
declaration or a definition is visible at the call site.

One definition per function name is allowed. Multiple declarations are
permitted as long as their parameter counts match, and if a declaration
and a definition both exist for the same name, their parameter counts
must also match. Function overloading is not allowed. All return types
and parameter types remain `int`.

## Changes from Stage 9.4

### Grammar

```ebnf
<function> ::= "int" <identifier> "(" [ <parameter_list> ] ")" ( <block_statement> | ";" )
```

This is the only grammar change — every other production is identical to
stage 9.4.

### Parser
- After parsing the header of an external declaration, peek the next
  token. `TOKEN_SEMICOLON` → declaration (no body). `TOKEN_LBRACE` →
  definition.
- Extend `FuncSig` with a `has_definition` flag so the parser can:
  - Reject a second definition of the same name.
  - Allow multiple declarations of the same name.
  - Enforce matching parameter counts across declarations and the
    definition.
- Drop the `parse_translation_unit` duplicate-name check (which currently
  rejects any repeated name). The signature table now owns that logic.
- The stage 9.4 undefined-call check still works — the table is
  populated by both declarations and definitions.

### AST
- No new node types. A pure declaration is an `AST_FUNCTION_DECL` with
  zero or more `AST_PARAM` children and **no** trailing `AST_BLOCK`.
  This keeps the AST delta minimal per project notes.

### Code Generation
- `codegen_function` skips nodes whose children contain no `AST_BLOCK`
  body. Declarations emit nothing.

### Documentation
- `docs/grammar.md` updated to reflect the `<function>` production
  change.

## Semantic Rules (from spec)

- Declaration and definition share the same identifier namespace.
- One definition per name.
- Parameter counts must agree across any declaration(s) and the
  definition for a given name.
- Declarations and definitions appear only at file scope.
- A call is valid if a declaration or definition is visible at the call
  site.

## Ambiguities / Decisions

1. **Spec typo (line 8).** "A function body also includes a function
   body" reads as intended to say "A function **definition** also
   includes a function body." Cosmetic; implementing the obviously
   intended meaning.
2. **"Function overriding is NOT allowed."** Read as *overloading*:
   each function name has exactly one parameter list and at most one
   definition.
3. **Redundant declarations.** The grammar allows `int f(int); int
   f(int);`. Not explicitly forbidden. Allowed, as long as parameter
   counts match — matches C semantics.
4. **Call to a function that is declared but never defined.** Spec
   says calls are allowed when a declaration is visible. Accepted by
   the compiler; a link-time unresolved symbol is the user's problem.
5. **Parameter name consistency.** Not required. Only parameter
   *count* is enforced ("matching parameter counts" from the spec
   notes).

## Planned File Updates

| File | Change |
|---|---|
| `include/parser.h` | `FuncSig` gains `has_definition` flag |
| `src/parser.c` | `;`-vs-`{` branch in function parsing; updated signature-table semantics; dup-check relaxation in `parse_translation_unit` |
| `src/codegen.c` | `codegen_function` skips bodyless declarations |
| `docs/grammar.md` | Sync `<function>` production |
| `test/valid/*.c` | declaration-before-definition, forward-call-via-prototype, decl-only-unused, multiple redundant declarations |
| `test/invalid/*.c` | parameter-count mismatch between decl and defn |

## Test Plan (preview)

- **Valid:**
  - Prototype before `main`, definition after (`int add(int,int); int main(){...add(40,2);...} int add(int a,int b){...}`).
  - Multiple redundant declarations followed by one definition.
  - Declaration of a function that is never called (must still compile).
  - Forward call that previously failed — now succeeds via a prototype.
- **Invalid:**
  - Declaration says 1 parameter, definition says 2 — compiler rejects.
  - Duplicate *definition* (already covered; must remain rejected).

## Next Step

Pause for confirmation of the plan, then implement in this order:
parser → codegen → tests → grammar doc → milestone/transcript → commit.
