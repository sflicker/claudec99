# Stage 107 Kickoff

## Summary of the Spec

Stage 107 closes three independent C99 gaps:

1. **`inline` function specifier** — currently causes a parse error. Fix: tokenize `inline` and consume it silently in declaration specifiers (same pattern as `restrict` in Stage 106). No codegen effect; parse-and-ignore is correct stub behaviour.

2. **`<assert.h>` stub** — the header does not exist. Fix: add `test/include/assert.h` with the `assert` macro using preprocessor features already present (`#expr` stringification, `__FILE__`, `__LINE__`, `fprintf`, `abort`). The macro must expand to a void expression and support the `NDEBUG` guard.

3. **`va_copy` codegen** — currently generates no code (silent no-op stub). Fix: implement proper 24-byte struct copy. Per the SysV AMD64 ABI, `va_list` is a 24-byte struct with three 8-byte words; copy via three `rax` moves.

All three changes are self-contained. No new AST nodes and no grammar productions change.

---

## Required Tokenizer Changes

In `include/token.h`, add `TOKEN_INLINE` to the token enum, adjacent to other keyword tokens:

```c
TOKEN_INLINE,
```

In `src/lexer.c`:
1. Add `inline` keyword recognition (parallel to existing `restrict` entry):
   ```c
   else if (strcmp(ident_buf.data, "inline") == 0) {
       token.type = TOKEN_INLINE;
       token.value = "inline";
       token.value_len = 6;
   }
   ```

2. Add `TOKEN_INLINE` to the `token_type_name()` display table:
   ```c
   case TOKEN_INLINE: return "'inline'";
   ```

---

## Required Parser Changes

In `src/parser.c`, locate the `while (1)` loop inside `parse_declaration_specifiers` (~line 3447).

Add a new branch that consumes `TOKEN_INLINE` and discards it, immediately after the `TOKEN_VOLATILE` branch and before the `TOKEN_EXTERN` / `TOKEN_STATIC` / `TOKEN_TYPEDEF` branch:

```c
} else if (parser->current.type == TOKEN_INLINE) {
    /* C99 §6.7.4 function-specifier: parse and ignore (no codegen effect) */
    parser->current = lexer_next_token(parser->lexer);
```

**No other parser changes needed.** `inline` never appears inside a declarator (unlike `restrict`), so pointer-declarator loops do not need updating.

---

## Required AST Changes

**None.** No new AST nodes.

---

## Required Code Generation Changes

In `src/codegen.c` (~line 3971), split the combined `va_end` / `va_copy` no-op branch:

1. Keep `va_end` as a no-op:
   ```c
   if (node->type == AST_BUILTIN_VA_END) {
       node->result_type = TYPE_VOID;
       return;
   }
   ```

2. Replace the `va_copy` no-op with the real implementation. `va_list` is a 24-byte struct with three 8-byte words; copy via three `rax` moves:
   ```c
   if (node->type == AST_BUILTIN_VA_COPY) {
       /* children[0] = dst (va_list), children[1] = src (va_list) */
       ASTNode *dst_node = node->children[0];
       ASTNode *src_node = node->children[1];
       LocalVar *lv_dst = codegen_find_var(cg, dst_node->value);
       LocalVar *lv_src = codegen_find_var(cg, src_node->value);
       if (!lv_dst || !lv_src) {
           compile_error("error: va_copy: operand is not a local variable\n");
       }
       int dst_off = lv_dst->offset;
       int src_off = lv_src->offset;
       /* Copy 24-byte va_list struct: three 8-byte moves via rax. */
       fprintf(cg->output, "    mov rax, [rbp - %d]\n",      src_off);
       fprintf(cg->output, "    mov [rbp - %d], rax\n",      dst_off);
       fprintf(cg->output, "    mov rax, [rbp - %d]\n",      src_off - 8);
       fprintf(cg->output, "    mov [rbp - %d], rax\n",      dst_off - 8);
       fprintf(cg->output, "    mov rax, [rbp - %d]\n",      src_off - 16);
       fprintf(cg->output, "    mov [rbp - %d], rax\n",      dst_off - 16);
       node->result_type = TYPE_VOID;
       return;
   }
   ```

   **Why rax is safe**: `va_copy` appears only in expression statements; no caller expects a value in rax after the call.

3. Create `test/include/assert.h`:
   ```c
   #ifndef CLAUDEC99_ASSERT_H
   #define CLAUDEC99_ASSERT_H

   #ifdef NDEBUG
   #define assert(expr) ((void)0)
   #else
   #include <stdio.h>
   #include <stdlib.h>
   #define assert(expr) \
       ((expr) ? (void)0 : (fprintf(stderr, \
           "assertion failed: %s, file %s, line %d\n", \
           #expr, __FILE__, __LINE__), abort(), (void)0))
   #endif

   #endif
   ```

---

## Test Plan

### Valid tests

- **`test_inline_func__0.c`** — inline function at file scope: `inline int square(int x) { return x * x; }` (exit: 0)
- **`test_static_inline__0.c`** — static inline function (exit: 0)
- **`test_extern_inline__0.c`** — extern inline declaration followed by definition (exit: 0)
- **`test_assert_pass__0.c`** — assert with true condition (exit: 0)
- **`test_assert_ndebug__0.c`** — assert with `NDEBUG` defined; `assert(0)` becomes no-op (exit: 0)
- **`test_va_copy_basic__0.c`** — va_copy with independent advancement; sum checked via two copies (exit: 0)
- **`test_void_implicit_return__0.c`** — regression: void function with implicit return at end (exit: 0)

### Invalid tests

- **`test_assert_fail__134.c`** — `assert(0)` aborts; expected exit non-zero. Place in `test/valid/` with suffix `__134` (SIGABRT = 6, shell exit = 128+6 = 134). Compile succeeds; runtime exits non-zero.

---

## Implementation Order

1. `include/token.h` — add `TOKEN_INLINE`
2. `src/lexer.c` — recognize `"inline"` → `TOKEN_INLINE`; add to display table
3. `src/parser.c` — consume `TOKEN_INLINE` in `parse_declaration_specifiers`
4. `src/codegen.c` — split va_end/va_copy; implement va_copy 24-byte copy
5. `test/include/assert.h` — new file
6. `src/version.c` — bump stage to `"01070000"`
7. Add test files
8. Build, run full test suite, self-host cycle
9. Update documentation (README.md, docs/grammar.md, docs/outlines/checklist.md; run update-supplemental-docs skill)

---

## Ambiguities and Decisions

**`inline` placement and semantics:** The spec explicitly states that `inline` never appears inside a declarator (unlike `restrict`), so pointer-declarator parsing loops do not need modification. Parse-and-ignore is the correct stub; actual inlining is out of scope.

**`va_copy` operand constraints:** Only local `va_list` variables are supported (found via `codegen_find_var`). Passing `va_list` as a pointer parameter is out of scope.

**Assert failure exit code:** SIGABRT (signal 6) results in shell exit code 128 + 6 = 134; use filename suffix `__134.c` for the test.

---

## Known Issues

None identified. The spec is clear and unambiguous. All three tasks are independent and non-overlapping.

