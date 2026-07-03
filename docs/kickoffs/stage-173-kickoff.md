# Stage 173 Kickoff: GNU Named Variadic Macros

## 1. Summary of the spec

A user reports that `cc99 -c net_helpers.c` fails with `error: include file not found: <arpa/inet.h>` when compiling a program with standard POSIX networking headers.

Investigation revealed two issues:

**Non-issue (expected behavior):** Without the `--sysinclude` flag, cc99 only searches the project's stub headers in `test/include/`; real system headers in `/usr/include` are opt-in. This is documented and working as designed.

**Real bug (preprocessor):** When compiling with `--sysinclude`, a genuine preprocessor defect blocks all networking headers. The GNU C library header `/usr/include/linux/stddef.h` (pulled in transitively by `sys/socket.h`, `netinet/in.h`, and `netdb.h`) defines:

```c
#define __struct_group(TAG, NAME, ATTRS, MEMBERS...) /* ... */
```

This uses the GNU "named variadic macro parameter" extension—an identifier immediately followed by `...` (e.g., `MEMBERS...`), which differs from the standard forms (bare `...` or comma-prefixed `...`). The preprocessor's `#define` parameter-list parser in `src/preprocessor.c` only recognized standard forms and rejected `MEMBERS...` with `error: expected ',' or ')' in macro parameter list`.

**Scope:** Fix the preprocessor to accept and correctly expand GNU named variadic macros, unblocking all system headers and networking code.

## 2. Required tokenizer changes

None.

## 3. Required parser changes

None.

## 4. Required AST changes

None.

## 5. Required code generation changes

None.

## 6. Preprocessor changes

### MacroDef struct extension

Add a `char *va_name` field to store the variadic parameter's substitution name:
- `"__VA_ARGS__"` for standard forms (`(...)` or `(a, b, ...)`).
- The given identifier for GNU named form (`(a, b, args...)`).
- NULL for non-variadic macros.

### Parameter-list parser update

After parsing an identifier in a macro parameter list, peek for `...` immediately following it (without requiring a preceding comma). If found:
- Treat it as the final variadic parameter.
- Record its name as `va_name` (not added to the fixed `params[]` array).
- Require `)` immediately after, matching existing anonymous-`...` error handling.

### Macro definition and expansion

- Modify `macro_define()` to accept a `va_name` parameter (owned, nullable). For variadic macros, default to `strdup("__VA_ARGS__")` if va_name is NULL. Properly free va_name on redefinition, undef, and table teardown.
- Update redefinition compatibility checks to compare va_name presence.
- Update the two macro-expansion call sites that hardcoded `ext_params[def->param_count] = "__VA_ARGS__"` to use `def->va_name`.
- Update the `#`/`##` replacement-text validator's special-case check for `__VA_ARGS__` to use the macro's actual effective va name.

### Predefined macro updates

All internal `macro_define()` call sites for predefined macros (`__STDC__`, `__x86_64__`, etc.) updated to pass NULL for the new va_name parameter (all are object-like, non-variadic).

## 7. Test plan

Two new portable integration tests under `test/integration/`:

- **test_pp_named_variadic_macro:** Bare-form GNU named variadic macro expansion.
  ```c
  #define REPEAT_TWICE(MEMBERS...) MEMBERS; MEMBERS
  ```
  Tests direct stringification and substitution mirroring the `__struct_group` shape.

- **test_pp_named_variadic_macro_fixed_param:** Mixed form with a fixed parameter before the named variadic.
  ```c
  #define LOG(fmt, args...) printf(fmt, args)
  ```
  Tests common GNU idiom matching actual system header patterns.

Manual verification (post-fix): With `--sysinclude`, all of `arpa/inet.h`, `errno.h`, `netdb.h`, `netinet/in.h`, `pthread.h`, `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/socket.h`, `sys/types.h`, `unistd.h` compile individually, and the full `net_helpers.c` program compiles cleanly. Full existing suite re-run: all portable, system-include, and optional-library tests pass.
