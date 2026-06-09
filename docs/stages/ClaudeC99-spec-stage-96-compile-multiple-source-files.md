# ClaudeC99 Stage 96 — Compile multiple source files per invocation

## Goal

Let the compiler accept **more than one source file on the command line** and
compile each one independently, in order. The driver compiles a single file at
a time through the full pipeline (read → preprocess → lex → parse → codegen →
write `.asm`), and **tears down every subsystem's heap allocations before
moving to the next file** so that each file starts from a clean slate. The next
file builds fresh lexer, parser, and code-generator state.

Today `src/compiler.c` rejects a second positional argument:

```c
} else if (!source_file) {
    source_file = argv[i];
} else {
    fprintf(stderr, "usage: ...\n");   /* <-- second file is an error */
    return 1;
}
```

and the single-file path it does support never frees the parser or
code-generator tables — there are no `parser_free`/`codegen_free` functions at
all. For a one-shot process that is harmless (the OS reclaims everything at
exit), but to compile N files in one process we must release each file's state
before the next, or memory grows unboundedly and stale state could leak across
files.

This is a driver / lifecycle stage. **No language features change** and no
program that compiles today should compile differently. A single-file
invocation must behave exactly as it does now (same `.asm`, same
`compiled: <src> -> <out>` line, same exit status).

## Background — what the pipeline allocates per file

Per-file heap that must be released after each compile:

| Subsystem | Owner | Freed today by | Owns |
|-----------|-------|----------------|------|
| Preprocessed source text | `char *preprocessed` (local) | `free()` | one buffer |
| Lexer | `Lexer lexer` (local) | `lexer_free` ✓ | `str_pool` Vec + `SourceFile` objects |
| AST | `ASTNode *ast` (local) | `ast_free` ✓ | the node tree |
| Parser | `Parser parser` (local) | **nothing** ✗ | `funcs`, `globals`, `typedefs`, `enum_consts`, `enum_tags`, `struct_tags`, `union_tags` (seven Vecs) |
| Code generator | `CodeGen cg` (local) | **nothing** ✗ | `locals`, `globals`, `break_stack`, `switch_stack`, `user_labels`, `string_pool`, `local_statics` (seven Vecs) + `util_strdup`'d label strings |
| Output file | `FILE *out` (local) | `fclose` ✓ | OS handle |

So the new work is: **add `parser_free` and `codegen_free`**, call them (and the
already-existing frees) after every file, and reset the global error state
between files.

## Task 1 — accept multiple positional source files

In `src/compiler.c`'s argument loop, collect every non-option argument into a
growable list of source files instead of a single `source_file` pointer (reuse
the same `realloc`-doubling idiom already used for `defines` and
`include_dirs`):

```c
const char **source_files = NULL;
int n_source_files = 0, source_files_cap = 0;
...
} else {
    /* positional argument: a source file */
    /* push argv[i] onto source_files (doubling realloc) */
}
```

- An option (anything starting with `-`/`--`, plus the existing `--version`,
  `--print-ast`, etc.) is still parsed exactly as today; only the final
  positional-argument branch changes.
- **Zero source files** remains the usage error it is today (print usage, exit
  1).
- Options apply uniformly to **all** files (the existing global flags
  `--print-ast`, `--print-tokens`, `-Werror`, `--max-errors=N`, `--sysroot=`,
  and the `-D`/`-I` lists are shared by every file). The sysroot-combined
  include paths are computed once, before the loop, and reused for each file.

## Task 2 — extract a per-file `compile_one_file` and loop over the files

Refactor the single-file body (everything from "Read source and preprocess"
through "compiled: ...") into a static helper:

```c
/* Compile one source file end-to-end. Returns 0 on success, 1 on failure.
 * Builds, uses, and fully tears down its own Lexer/Parser/CodeGen/AST and
 * the preprocessed buffer before returning. */
static int compile_one_file(const char *source_file,
                            int print_ast, int print_tokens,
                            int warnings_are_errors,
                            const char **defines, int n_defines,
                            const char **include_dirs, int n_include_dirs);
```

`main` then loops:

```c
int overall_status = 0;
for (int f = 0; f < n_source_files; f++) {
    reset_error_state();                 /* Task 5 */
    if (compile_one_file(source_files[f], ...) != 0)
        overall_status = 1;
}
... free argv-derived lists ...
return overall_status;
```

Semantics:

- **Independent compilation.** Each file is read, preprocessed, lexed, parsed,
  and code-generated on its own fresh subsystems. A failure in one file
  (missing file, parse errors, output-open failure) is reported and sets the
  overall exit status to 1, **but does not stop the remaining files** from being
  attempted. (This matches the common `cc a.c b.c` behavior: every file is
  attempted; the process exits non-zero if any failed.)
- **Output naming is per file**, exactly as today: `make_output_path` derives
  each `<name>.asm` from that file's path, and each success prints its own
  `compiled: <src> -> <out>` line. Because each file gets a brand-new `CodeGen`,
  per-file label counters (`Lstr0`, `.L_switch_*`, …) restart at zero — correct,
  since the `.asm` files are independent.
- **`--print-tokens` / `--print-ast`** apply to each file in sequence (the
  existing early-return modes move inside `compile_one_file`). These diagnostic
  modes are essentially never used with multiple files; per-file sequential
  output with no extra separators is sufficient and needs no new golden test.
- **Single-file invocation is unchanged**: the loop runs once and the observable
  output/exit status is byte-for-byte what it is today.

## Task 3 — add `parser_free`

Declare in `include/parser.h` and define in `src/parser.c`:

```c
void parser_free(Parser *parser);
```

It must `vec_free` the seven Vecs the parser owns: `funcs`, `globals`,
`typedefs`, `enum_consts`, `enum_tags`, `struct_tags`, and `union_tags`.

**Do not free the element strings.** Since stage 95-10 every `name`/`tag` field
in those entries is a `const char *` pointing into **lexer-owned** string-pool
storage (`Token.value`), not memory the parser owns. Freeing the Vec backing
buffers is the parser's whole responsibility; the lexer's `str_pool` is freed by
`lexer_free`. (The `Type *` pointers in `StructTag`/`UnionTag`/`TypedefEntry`
are part of the shared type arena — see *Out of scope*.)

## Task 4 — add `codegen_free` with clean string ownership

Declare in `include/codegen.h` and define in `src/codegen.c`:

```c
void codegen_free(CodeGen *cg);
```

It must `vec_free` the seven Vecs the code generator owns: `locals`, `globals`,
`break_stack`, `switch_stack`, `user_labels`, `string_pool`, and
`local_statics`. (The inner `Vec entries` inside each `SwitchCtx` on
`switch_stack` is already freed during emission — stage 95-12 — so by the time
codegen completes `switch_stack` holds no live inner buffers; `codegen_free`
just releases the outer backing store.)

**Generated label strings — give them single ownership.** Codegen calls
`util_strdup` in two places and the results currently have *mixed* ownership,
which makes per-field freeing unsafe:

- `LocalStaticVar.static_label` (src/codegen.c ~line 4006) — always owned
  (`util_strdup`'d).
- `GlobalVar.init_label` (~line 5081) — **sometimes** owned (`util_strdup`'d
  from a temp) and **sometimes** a non-owned pointer into AST/lexer storage
  (`init->value`, ~line 5065) or `NULL`.
- `user_labels` entries push `&node->value` — **non-owned** AST pointers, never
  to be freed.

Blindly freeing these fields would risk double-frees and freeing non-owned
storage. Instead, route every codegen `util_strdup` through a small
codegen-owned intern helper so all generated strings have one clear owner:

```c
/* Add to CodeGen: */
Vec owned_strings;   /* char *  — every codegen-generated string */

/* Helper (src/codegen.c): strdup `s`, record it for cleanup, return it. */
static const char *codegen_intern(CodeGen *cg, const char *s);
```

Replace the two `util_strdup(...)` call sites with `codegen_intern(cg, ...)`.
`codegen_init` does `vec_init(&cg->owned_strings, sizeof(char *))`;
`codegen_free` frees every recorded pointer, then `vec_free`s `owned_strings`
along with the other seven Vecs. `user_labels` is freed as a Vec only (its
elements stay untouched — they belong to the AST/lexer). This achieves "free
everything the code generator allocated" with no ownership ambiguity.

## Task 5 — reset global error state between files

`util.c` exposes process-global compilation state (`include/util.h`):
`g_error_count`, `g_error_src_file`, `g_error_src_line`, `g_error_src_col`,
`g_error_jmp_valid`. `parse_translation_unit` increments `g_error_count` and the
driver checks it after parsing. If file 1 left `g_error_count > 0`, file 2 would
inappropriately inherit it.

Add a small reset routine (declared in `include/util.h`, defined in
`src/util.c`) and call it at the top of each loop iteration:

```c
void reset_error_state(void);   /* zeroes g_error_count, g_error_src_*,
                                   g_error_jmp_valid */
```

**Do not reset configuration globals** `g_max_errors` and
`g_warnings_are_errors` — those come from the command line and apply to all
files.

The preprocessor needs no reset: it builds and frees a local `MacroTable` per
call and injects predefined/command-line macros each time, so it holds no
persistent cross-file state. The base type singletons in `type.c` are immutable
constants and likewise carry no per-file state.

## Out of scope — do NOT do these in this stage

- **Freeing the `Type` arena.** Derived types (`type_pointer`, `type_array`,
  `type_function`, `type_struct`, `type_const_copy`, …) are `malloc`'d and
  shared between the AST and the code generator; there is no arena and
  `ast_free` does not free them, so they are a process-lifetime leak today. This
  stage does **not** address that — base types are stateless singletons and
  derived types are rebuilt fresh per file, so the leak does **not** corrupt
  cross-file compilation (it is purely unreclaimed memory). A dedicated
  per-compilation **type arena** is a good follow-up stage; flag it but leave it
  out here to keep the change minimal.
- **Linking / multi-file objects.** This stage emits one independent `.asm` per
  source file. It does **not** link them, merge translation units, share symbols
  across files, or emit `.o` files. Each file is a self-contained translation
  unit, exactly as a single-file invocation is today. (`bin/cc99` and the
  integration runner already drive assembling/linking per file and need no
  change.)
- **`-o` output naming.** Output paths stay derived from each source name via
  `make_output_path`. No `-o <file>` option is added (it would be ambiguous with
  multiple inputs).
- **Changing `bin/cc99` or the integration runner.** Both already invoke
  `ccompiler` once per file in a shell loop, so they keep working unchanged. The
  new capability is the compiler *also* accepting several files at once.

## Bootstrap caveats

The compiler must continue to self-host (C0→C1→C2). Watch the usual C0 patterns
when touching `Vec` code (`docs/fixed-capacity-inventory.md`):

- **C0 cannot parse `*(T **)vec_get(...)`** — split a cast-of-dereference into
  two statements (`char **pp = (char **)vec_get(...); char *s = *pp;`). The new
  `codegen_free` loop over `owned_strings` and the `reset_error_state` work both
  touch this pattern; reproduce any suspicious construct against
  `build/ccompiler-c0` before a full `./build.sh --mode=self-host`.
- Keep any capacity arithmetic on `size_t` locals (unsigned `div`), as the
  existing `Vec`/`StrBuf` code already does.

A natural self-host smoke test: the bootstrap already compiles all ~12 source
modules one at a time. After this stage, optionally confirm the compiler can
take several of them in a **single** invocation (e.g.
`ccompiler -I include -I test/include src/lexer.c src/parser.c`) and produce
both `.asm` files — exercising the per-file teardown loop on real input.

## Tests

- **Multi-file success (integration).** Add a `test/integration/` case (or a
  dedicated driver test) that invokes `ccompiler` **once** with two independent
  source files and asserts both `.asm` outputs are produced, assemble, link, and
  run with the expected results. This is the headline new capability.
- **Independent state between files.** Include two files that each define a
  symbol with the **same name** (e.g. both define a `static` helper `foo` or a
  `struct Node`) to prove the second file's parser/codegen tables are not
  polluted by the first — both must compile cleanly and produce correct,
  independent output.
- **Per-file failure isolation.** A driver/integration test where the first
  file has a compile error and the second is valid: the process must report the
  first file's error, still emit the second file's `.asm`, and exit non-zero
  overall. (If the existing harness can't express "compile error in one of
  several files," a small shell-driven test under `test/integration/` is
  acceptable; otherwise document the manual check.)
- **Single-file regression.** The entire existing suite must pass unchanged —
  single-file invocation is the N=1 case of the new loop and must be
  byte-for-byte identical.
- **No-leak smoke (optional but recommended).** If `valgrind` is available,
  compile two files in one invocation under valgrind and confirm no
  *new* growth from the parser/codegen tables (the documented `Type`-arena leak
  is expected and out of scope).

## Docs (at stage close, after all tests pass)

- **`README.md`** — update the `Usage` line and the frontend/usage prose to show
  that `ccompiler` accepts one *or more* `<source.c>` files and compiles each to
  its own `.asm`; add a "Through stage 96" capability blockquote. Note that
  multi-file linking/objects remain out of scope. Refresh the aggregate test
  totals.
- **`docs/grammar.md`** — no grammar change (driver-only stage); confirm and
  state that no update is needed.
- Run the **`update-supplemental-docs`** skill to refresh the checklist,
  parse-tree, and status/features snapshots through stage 96 (note the new
  `parser_free`/`codegen_free`/`reset_error_state` lifecycle functions and the
  `owned_strings` ownership change in codegen).
- **`docs/self-compilation-report.md`** — record the stage-96 self-host run
  (issues found/fixed + the C0/C1/C2 result table), per the standard workflow.
- Update **`src/version.c`** (`VERSION_STAGE "00960000"`).
