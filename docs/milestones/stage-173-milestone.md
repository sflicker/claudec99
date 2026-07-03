# Milestone Summary

## 173: GNU Named Variadic Macro Parameters - Complete

stage-173 ships support for the GNU extension: named variadic macro parameters (e.g. `#define M(x, args...) ...` where `args` replaces the standard `__VA_ARGS__`), unblocking compilation of real networking and system headers under `--sysinclude`.

- Preprocessor: Added `char *va_name` field to `MacroDef` struct to track the variadic parameter's substitution name (`"__VA_ARGS__"` for standard forms, or the user-given name for GNU named form); updated `#define` parameter-list parser to recognize `NAME...` (identifier immediately followed by `...`) as the final variadic parameter; `macro_define()` now accepts and manages a `va_name` parameter (with proper cleanup on redefinition/`#undef`); both macro-expansion substitution sites and the `#`/`##` validator now use the macro's actual va_name instead of hardcoded `"__VA_ARGS__"`; all 30 predefined-macro call sites updated with NULL for the new parameter.
- Tests: Two new portable integration tests (`test_pp_named_variadic_macro`, `test_pp_named_variadic_macro_fixed_param`). Full suite: 2074 portable + 191 system-include + 2 optional-library pass.
- Docs: `docs/grammar.md` documents named variadic support in preprocessor notes; `docs/outlines/checklist.md` flipped the done sub-item under the GNU extensions TODO.
- Self-host: C0→C1→C2 verified (version `01730000`, all tests passing at every stage).
- Notes: This fix enables use of real networking headers (arpa/inet.h, netinet/in.h, netdb.h) via `--sysinclude`; those headers transitively include linux/stddef.h which defines `__struct_group` using GNU named variadic syntax. The GNU `__VA_OPT__` extension remains unimplemented.
