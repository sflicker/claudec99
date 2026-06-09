# Stage 96 Kickoff

**Stage 96:** Compile multiple source files per invocation

**Spec:** docs/stages/ClaudeC99-spec-stage-96-compile-multiple-source-files.md

---

## Summary

This is a **driver/lifecycle stage** — no language features change. The goal is to let the compiler accept more than one source file on the command line and compile each one independently in order, tearing down heap allocations between files.

Today `ccompiler` accepts a single source file and rejects a second positional argument. The code never calls `parser_free()` or `codegen_free()`, relying on OS cleanup at exit. To support N-file compilation in a single process, we must:

1. Collect multiple source files instead of a single `source_file` pointer.
2. Extract per-file compilation logic into a helper, then loop over all files.
3. Tear down the parser and code-generator tables after each file.
4. Reset global error state between files.
5. Achieve single-file byte-for-byte identical behavior (the N=1 case of the loop).

---

## Tokenizer changes

**None.** The lexer already owns and properly frees `str_pool` via `lexer_free()`, which is called after every file today. No changes to tokenization.

---

## Parser changes

**Add `parser_free` function** (`include/parser.h`, `src/parser.c`):

- Declare `void parser_free(Parser *parser)`.
- Call `vec_free` on the seven Vecs the parser owns: `funcs`, `globals`, `typedefs`, `enum_consts`, `enum_tags`, `struct_tags`, `union_tags`.
- Do **not** free element strings; they are in lexer-owned storage (Token.value → str_pool).

---

## AST changes

**None.** AST allocation/freeing is already via `ast_free()`, which is called today. No changes to AST structure.

---

## Code generation changes

**Add `codegen_free` function with string ownership cleanup** (`include/codegen.h`, `src/codegen.c`):

1. **Add `Vec owned_strings`** to `CodeGen` to track every string the code generator allocates.

2. **Add static helper `codegen_intern(CodeGen *cg, const char *s)`**:
   - `strdup` the input string.
   - Record the pointer in `owned_strings`.
   - Return the duplicated pointer.

3. **Replace two `util_strdup` call sites** with `codegen_intern`:
   - `LocalStaticVar.static_label` (~line 4006).
   - `GlobalVar.init_label` (~line 5081).

4. **Define `codegen_free(CodeGen *cg)`**:
   - Free every string recorded in `owned_strings`.
   - Call `vec_free` on all eight Vecs: `locals`, `globals`, `break_stack`, `switch_stack`, `user_labels`, `string_pool`, `local_statics`, `owned_strings`.
   - Do **not** free `user_labels` elements (they are non-owned AST/lexer pointers).

**Note:** The Type arena (derived Type allocations) is left as a known process-lifetime leak out of scope for this stage.

---

## Driver / main() changes

**Task 1 — Accept multiple positional source files** (`src/compiler.c`):

- Replace single `source_file` pointer with growable `source_files` array.
- Use `realloc`-doubling idiom (as already used for `defines`, `include_dirs`).
- Zero files remains a usage error; options apply uniformly to all files.

**Task 2 — Extract `compile_one_file` and loop** (`src/compiler.c`):

- Extract per-file body (read → preprocess → lex → parse → codegen → write) into static helper.
- Signature: `static int compile_one_file(const char *source_file, int print_ast, int print_tokens, int warnings_are_errors, const char **defines, int n_defines, const char **include_dirs, int n_include_dirs)`.
- Returns 0 on success, 1 on failure; calls `lexer_free`, `ast_free`, `parser_free`, `codegen_free`, `free(preprocessed)`.
- `main` loops over all files, calling `reset_error_state()` before each.
- One file's failure does not stop subsequent files; overall exit status is 1 if any file failed.
- Single-file invocation is byte-for-byte unchanged.

**Task 5 — Reset global error state** (`include/util.h`, `src/util.c`):

- Declare/define `void reset_error_state(void)`.
- Zero `g_error_count`, `g_error_src_file`, `g_error_src_line`, `g_error_src_col`, `g_error_jmp_valid`.
- Do **not** reset `g_max_errors` or `g_warnings_are_errors` (command-line config, shared across files).

**Version update** (`src/version.c`):

- Set `VERSION_STAGE` to `"00960000"`.

---

## Test plan

1. **Multi-file success (integration).** Invoke `ccompiler` once with two independent source files; both `.asm` outputs must be produced and assemble/link/run correctly.

2. **Independent state between files.** Two files each define a symbol with the same name (e.g. `static` helper `foo`, `struct Node`); both must compile cleanly and produce correct independent output.

3. **Per-file failure isolation.** First file has compile error, second is valid: first error is reported, second `.asm` is still emitted, exit code is 1. (Requires `--max-errors=0` or a custom shell-driven test.)

4. **Single-file regression.** Entire existing suite must pass unchanged; N=1 case of the loop is byte-for-byte identical.

5. **No-leak smoke test (optional).** Under `valgrind`, compile two files in one invocation and confirm no new unbounded growth from parser/codegen tables (Type arena leak is expected and out of scope).

---

## Implementation order

1. `util.h` / `util.c` → `reset_error_state()`.
2. `parser.h` / `parser.c` → `parser_free()`.
3. `codegen.h` / `codegen.c` → `owned_strings`, `codegen_intern()`, `codegen_free()`.
4. `src/compiler.c` → multiple source files, `compile_one_file()` loop.
5. `src/version.c` → stage number.
6. Integration tests.
7. Documentation refresh (README, grammar.md confirmation, supplemental docs, self-host report).

---

## Ambiguities / Notes

- **No grammar change.** This is a driver-only stage; `docs/grammar.md` is unchanged.

- **Bootstrap caveat:** C0 cannot parse `*(T **)vec_get(...)`; split cast-of-dereference into two statements. Both `codegen_free` and `reset_error_state` touch this pattern.

- **Per-file failure isolation behavior:** With `--max-errors=1` (default), a single error causes `exit(1)` before `longjmp`, so only one file is attempted. With `--max-errors=0`, failures in file 1 do not stop file 2 from being attempted. Tests should use the latter or manual shell-driven approach.

- **Type arena leak:** Derived types are rebuilt fresh per file and are not freed. This is a known process-lifetime leak flagged for a dedicated follow-up stage but left out here to keep the change minimal.

- **Linking / multi-file objects:** Each file compiles to an independent `.asm`. This stage does not link them, merge translation units, or share symbols across files. `bin/cc99` and the integration runner already drive per-file assembly/linking and need no change.

- **Output naming:** Stays derived from each source name via `make_output_path()`. No `-o` option is added.
