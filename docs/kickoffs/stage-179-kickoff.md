# Stage 179 Kickoff: x86-64 Mach-O Cross-Compile Target (Phase 0B)

## Derived Values

| Item | Value |
|------|-------|
| STAGE_LABEL | stage-179 |
| STAGE_DISPLAY | stage-179 |
| Spec | `docs/stages/ClaudeC99-spec-stage-179-x86_64-macos-target.md` |
| New target enum | `TARGET_X86_64_LINUX`, `TARGET_X86_64_MACOS`, `TARGET_ARM64_MACOS` |
| New CodeGen field | `int target_macho;` |
| VERSION_STAGE | `01790000` (stage 178 left it at `01770000`) |
| Expected output | `nasm -f macho64` plus `clang -arch x86_64`, run under Rosetta 2 |

## 1. Spec Summary

Stage 179 is Phase 0B of the ARM64/macOS roadmap. It adds an `x86_64-macos` target so the compiler, running on an Apple Silicon host, emits x86-64 NASM that assembles to Mach-O64 and runs under Rosetta 2.

The backend is otherwise unchanged. Only two things differ from the Linux output:

1. C-visible symbols get a `_` prefix on Mach-O (`main` becomes `_main`, `myvar` becomes `_myvar`). Internal labels (`Lstr*`, `Lfc*`, `Lfp_*`, `Lstatic_*`, `Lcompound_*`, `.L*`) do not.
2. The ELF-only `section .note.GNU-stack` footer is omitted.

The spec adds no tokenizer, parser, AST, or grammar changes. `--target=arm64-macos` is accepted but exits with a "not yet implemented" error. ARM64 codegen is Phase 1+, and the macOS test runners are Phase 0C.

## 2. Required Tokenizer Changes

None. The spec does not touch the lexer.

## 3. Required Parser Changes

None. The spec does not touch the parser or grammar (`docs/grammar.md` stays as is).

## 4. Required AST Changes

None.

## 5. Required Code Generation Changes

### 5a. Target plumbing (non-codegen files)

- **`include/target.h`** (new): `Target` enum as listed above, with an include guard `CCOMPILER_TARGET_H`.
- **`src/compiler.c`**:
  - Parse `--target=x86_64-linux|x86_64-macos|arm64-macos` after the flag loop. Unknown values are an error. `arm64-macos` is accepted and then exits with a "not yet implemented" error.
  - Auto-detect when `--target` is omitted via `#if defined(__APPLE__)`.
  - After `codegen_init(&cg, out)`, set `cg.target_macho = (target == TARGET_X86_64_MACOS);`.
  - Add `--target` to `--help`.
- **`include/codegen.h`**: add `int target_macho;` to `CodeGen`.
- **`src/version.c`**: `VERSION_STAGE` to `"01790000"`.
- **`build.sh`**: add `HOST_OS=$(uname -s)` and `HOST_ARCH=$(uname -m)`. In `do_bootstrap_build()`, select `NASM_FMT`, `LINK_CMD`, and `COMPILER_TARGET` per host. Darwin uses `macho64`, `(clang -arch x86_64)`, and `--target=x86_64-macos`. Linux keeps `elf64`, `(gcc -no-pie)`, and `--target=x86_64-linux`.

### 5b. `src/codegen.c`

- **`sp(cg)` helper** near the top of the file. It returns `"_"` when `target_macho` is set and `""` otherwise. Every `fprintf` that emits a C-visible name switches from `"%s"` to `"%s%s"`.
- **Category A, `global` and `extern`**: 5 sites (around lines 6622, 7099, 7122, 7562, 7835).
- **Category B, label definitions**: about 12 sites. This covers function labels (around 6623), global `db`/`resb`/`dq`/`dd` labels (around 7569–7817), and BSS labels.
- **Category C, `call`**: 3 sites (around 4076, 4123, 4161).
- **Category D, `[rel name]`**: 9 sites (around 1244–3249).
- **Category E, block-scope static `dq` RHS**: 4 sites in `codegen_emit_local_statics` (around 8087–8107). The label (`sv->label`, a `Lstatic_` name) is never prefixed. Only the RHS C name is.
- **Category F, `.note.GNU-stack`**: wrap the footer (around line 8189) in `if (!cg->target_macho)`.

Internal labels never use `sp()`.

### 5c. Known gaps in the spec table (see Open Items)

The spec table does not cover every site that needs the prefix or needs a change to how it is handled. Those are listed in Open Items 1 and 2.

## 6. Test Plan

The spec lists no tests. The existing suites cannot run on this macOS host: the test runners are ELF/Linux-only (Phase 0C), and macOS has no `timeout` command, so `build.sh` bootstrap and self-host cannot run here either. The proposed verification follows.

- **(a) Linux regression, byte-identical**: before and after the change, compile every `test/**/*.c` with `--target=x86_64-linux` and diff the `.asm` output. The diff must be empty. This guards the Linux path.
- **(b) macOS end to end**: for a sample of `test/valid` programs, run `--target=x86_64-macos`, assemble with `nasm -f macho64`, link with `clang -arch x86_64`, and run under Rosetta 2. Compare exit codes and stdout with the expected results.
- **(c) Spec verification script**: run the spec's Verification block. Expect `_main:` present, no `.note.GNU-stack` in the macOS output, exit 42, and auto-detect matching the explicit macOS output.
- **(d) Flag handling**: `--target=bogus` errors, `--target=arm64-macos` exits with "not yet implemented", and `--help` lists `--target`.

## Implementation Plan

Order (from the launcher's plan):

1. `include/target.h`
2. `src/compiler.c` flag parsing and auto-detect
3. `include/codegen.h` field
4. `src/codegen.c` `sp()` helper and symbol sites (Categories A–F)
5. `build.sh` platform selection
6. `src/version.c` bump
7. Verification (plan section 6)
8. Docs
9. Commit (only when the user asks)

Pause after each step for review.

## Open Items (open, pending user decision)

These are recorded for the user to decide. Nothing here has been decided yet.

1. **Spec table incomplete for shared helpers.** `emit_load_global`, `emit_store_global`, `emit_load_fp_global`, and `emit_store_fp_global` (around codegen.c 559, 567, 815, 837) emit `[rel %s]`. They are called with both `gv->name`, which is a C symbol and needs `_`, and `lv->static_label`, which is `Lstatic_*` and must not get `_`. The proposed fix is a prefix argument on each helper. The FP-literal global initializer label definitions (around codegen.c 7798–7815, the `%s: %s %.*s` and `%s: %s %s` forms) are also missing from the spec table.
2. **`gv->init_label` is ambiguous (spec row around 7813).** The spec says to prefix `init_label` when `is_label_init` is set. But `init_label` can hold a C function or global name (needs `_`), or an internal `Lstr*` string-pool label or a `codegen_intern()` FP label (must not be prefixed, e.g. `char *s = "abc";`). A distinguishing mechanism is needed, such as a flag set at the assignment sites (around codegen.c 7184–7298) or a check on the label prefix.
3. **Auto-detect uses `__APPLE__`, which ClaudeC99 never defines.** `src/preprocessor.c` predefines `__linux__` (line 2425) and not `__APPLE__`. A self-hosted ccompiler built on macOS would therefore default to the Linux target when `--target` is omitted. `build.sh` passes `--target` explicitly, so bootstrap is not affected. The `#if defined(__APPLE__)` check is host-compiler-only and is fine for the C compiler building itself, but the self-hosted binary's default needs a decision.
4. **No tests in the spec.** The proposed verification in section 6 needs sign-off. In particular, byte-identical `.asm` regression across all test sources is the main safety net, since the suites cannot run on macOS.
5. **Self-host cannot run on this host.** No `timeout` command and ELF-only test runners. Self-host verification has to wait for Phase 0C or run on a Linux machine. Decide whether stage 179 is accepted on the partial verification above.
6. **VERSION_STAGE drift.** Stage 178 did not bump `VERSION_STAGE` (it stayed at `01770000`). Stage 179 sets `01790000`, which skips a value. Decide whether to leave it or set `01780000` retroactively.
7. **Spec line numbers may have drifted.** The spec's line references (for example 7813) are close but not exact. For example, the `init_label` emission currently sits at codegen.c 7814. Re-derive line numbers from the code during implementation rather than trusting the table.

## Notes

- `docs/kickoffs/stage-177-kickoff.md` is the style reference for this file.
- This kickoff covers planning only. No compiler source, tests, or `build.sh` changes are made by this artifact.
