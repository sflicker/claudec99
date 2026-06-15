# ClaudeC99 Stage 126 — Tentative Definitions

## Goal

Implement C99 §6.9.2 tentative definitions for file-scope variables.

---

## Background

C99 §6.9.2p2:

> A declaration of an identifier for an object that has file scope without
> an initializer, and without a storage-class specifier or with the storage-class
> specifier `static`, constitutes a *tentative definition*. If a translation unit
> contains one or more tentative definitions for an identifier, and the translation
> unit contains no external definition for that identifier, then the behavior is
> exactly as if the translation unit contains a file-scope declaration of that
> identifier, with the composite type as of the end of the translation unit, with
> an initializer equal to 0.

In practice this means:

```c
int x;        /* tentative definition */
int x;        /* another tentative definition — legal, merges into one */
int x = 5;   /* external definition that completes the tentative definition */
```

All three lines refer to the same object. The compiler must not error on the
second `int x;` or the `int x = 5;` line.

Currently, the second declaration causes:
```
error: duplicate global declaration 'x'
```

---

## Root Cause

`parser_register_global` in `src/parser.c` (line ~202) treats any two
`SC_NONE + SC_NONE` file-scope declarations as a duplicate definition error,
regardless of whether either has an initializer:

```c
if (existing->storage_class != SC_EXTERN && sc != SC_EXTERN) {
    PARSER_ERROR(parser, "error: duplicate global declaration '%s'\n", name);
}
```

It does not distinguish between:
- tentative (no initializer) + tentative → legal
- tentative + definition (with initializer) → legal (completes the definition)
- definition + definition → error (duplicate definitions)

---

## Fix

### `include/parser.h` — Add `is_defined` to `GlobalObjSig`

```c
typedef struct {
    const char *name;
    TypeKind kind;
    StorageClass storage_class;
    struct Type *full_type;
    int is_defined;   /* 1 if this entry has a complete definition (with initializer) */
} GlobalObjSig;
```

### `src/parser.c` — `parser_register_global`: add `has_init` parameter

Change the signature to:
```c
static void parser_register_global(Parser *parser, const char *name,
                                   TypeKind kind, StorageClass sc,
                                   Type *full_type, int has_init)
```

Update the `SC_NONE + SC_NONE` block:

```c
if (existing->storage_class != SC_EXTERN && sc != SC_EXTERN) {
    /* Both are non-extern (external or static linkage). */
    if (ex_is_static) {
        /* Static tentative: two tentative defs are allowed; two definitions are not. */
        if (existing->is_defined && has_init) {
            PARSER_ERROR(parser, "error: duplicate definition of '%s'\n", name);
        }
    } else {
        /* External linkage: same rules. */
        if (existing->is_defined && has_init) {
            PARSER_ERROR(parser, "error: duplicate definition of '%s'\n", name);
        }
    }
    /* Update is_defined if this declaration is a complete definition. */
    if (has_init) existing->is_defined = 1;
    return;
}
```

Also set `is_defined` when creating a new entry:
```c
new_g.is_defined = has_init;
```

Update all three call sites to pass `has_init`:
- Line ~3612: `parser_register_global(parser, d.name, TYPE_POINTER, sc, fp_type, has_init)`
- Line ~3691: `parser_register_global(parser, d.name, obj_kind, sc, reg_full_type, has_init)`
- Line ~3882: `parser_register_global(parser, d2.name, k2, sc, reg_ft2, has_init2)`

`has_init` at each call site is true if a child was added to the `decl` AST node
(i.e., the declaration has an initializer). Check `decl->child_count > 0` just
before or at the call site, or pass it as a local variable computed during parsing.

### `src/codegen.c` — `codegen_add_global`: handle duplicate names

When `codegen_add_global` is called for a name that already exists in `cg->globals`,
it should:
- If new entry has no initializer (tentative): skip — the existing entry wins.
- If new entry has an initializer (definition): update the existing entry with
  the initializer data (override BSS with .data).

Add a lookup at the top of `codegen_add_global`:

```c
static void codegen_add_global(CodeGen *cg, ASTNode *decl) {
    /* Tentative definitions: if a GlobalVar with this name already exists,
     * the new entry either updates it (if it has an initializer) or is skipped. */
    GlobalVar *existing_gv = codegen_find_global(cg, decl->value);
    if (existing_gv) {
        if (decl->child_count == 0) return;   /* tentative — existing entry wins */
        /* New entry has initializer: update the existing entry in-place. */
        /* ... populate existing_gv fields from decl ... */
        /* Fall through is not needed — set fields on existing_gv and return. */
        /* [apply init logic to existing_gv instead of new_gv] */
        return;
    }
    /* ... rest of existing logic ... */
}
```

Alternatively (simpler): parse and populate `new_gv` as before, then if an
existing entry was found, copy `new_gv`'s init fields into it if `is_initialized`.

---

## Call-site: which `has_init` value to pass

### Pointer declarations (line ~3612)

```c
/* At this point, the decl node has been built but no child added yet.
 * has_init is determined by whether TOKEN_ASSIGN was consumed. */
int has_init = (decl->child_count > 0);
parser_register_global(parser, d.name, TYPE_POINTER, sc, fp_type, has_init);
```

### Scalar / struct / union declarations (line ~3691)

Same pattern — compute `has_init` from `decl->child_count > 0` just before the call.

### Comma-separated multi-declarator list (line ~3882)

Each sub-declarator is registered independently. Pass `has_init2 = (next_decl->child_count > 0)`.

---

## Tests

**`test/valid/declarations/test_tentative_def_two_tentative__0.c`**:
```c
int x;
int x;
int main(void) { return x; }
```
Expected exit: 0 (x is zero-initialized as per C99 §6.9.2).

**`test/valid/declarations/test_tentative_def_then_init__0.c`**:
```c
int x;
int x = 7;
int main(void) { return x - 7; }
```
Expected exit: 0.

**`test/valid/declarations/test_tentative_def_init_then_tentative__0.c`**:
```c
int x = 7;
int x;
int main(void) { return x - 7; }
```
Expected exit: 0 (definition comes first; tentative after is a no-op).

**`test/valid/declarations/test_tentative_def_struct__0.c`**:
```c
struct Point { int x; int y; };
struct Point p;
struct Point p;
int main(void) { return p.x + p.y; }
```
Expected exit: 0.

**`test/invalid/declarations/test_duplicate_definition.c`**:
```c
int x = 1;
int x = 2;
int main(void) { return 0; }
```
Expected: compile error.

---

## Version

Bump `VERSION_STAGE` in `src/version.c` to `"01260000"`.

---

## Out of Scope

- Incomplete array types in tentative definitions (`int arr[];` at file scope) —
  handled in Stage 129.
- Tentative definitions for static-duration struct arrays with partial initializers —
  existing behavior (zero-pad) is already correct; the tentative-definition rule
  only affects the parser's duplicate-detection logic.
- Cross-file tentative definitions — this compiler compiles one file at a time;
  multi-file semantics are out of scope until object file emission is implemented.
