# Milestone Summary

## Stage 183: ARM64 Comparisons and Control Flow (Phase 3) - Complete

stage-183 extends the experimental native ARM64 backend (`--target=arm64-macos`) from straight-line integer code to comparisons, logical operators, the conditional operator, selection and iteration statements, and jumps.

- Tokenizer: No changes.
- Grammar/Parser: No changes (`docs/grammar.md` unchanged).
- AST/Semantics: No changes. The backend consumes the existing AST. Stage 182 rejections for comparison, logical, and logical-not operators are removed. Still rejected with `arm64-macos: <construct> not yet supported`: parameters and calls, globals, pointers and aggregates, unsigned and narrow types, floating point, and varargs.
- Codegen: `CodeGenArm64` (`include/codegen_arm64.h`, `src/codegen_arm64.c`) now handles signed comparisons, short-circuit `&&`/`||`, `!`, and `?:`; `if`, `if`/`else`, `while`, `do`-`while`, and `for` (loop-scoped init); `break`/`continue` through a jump stack; `switch` with linear compare dispatch; and `goto` with labels. Labels use `L<n>_<kind>` for internal targets and `L<func>_usr_<name>` for user labels. Duplicate and undefined labels report the x86 messages. The frame pre-scan now covers all nested statements. x86 targets are unchanged.
- Tests: 4 new valid tests (long-versus-int comparison, switch with out-of-immediate-range case values, `continue` inside a switch, and a 272-byte-deep local slot). The ARM64 manifest gains a tier-3 section of 366 entries. The integration test `test_arm64_skeleton_asm` now uses `floating_point.c` as its unsupported-construct case. Native ARM64 tier 3: 439 of 1297 in scope, 439/439 pass; tier 2 73/73; tier 1 5/5. Full portable suite 2174/2174 pass (249 unit, 1297 valid, 261 invalid, 196 integration, 50 print-AST, 100 print-tokens, 21 print-asm).
- Docs: `VERSION_STAGE` set to `01830000`; README gains a Through Stage 183 entry and updated test totals; checklist gains a Stage 183 section; self-compilation report updated.
- Notes: Two tests that fail on x86 because of a known defect (`control_flow/test_switch_long_discriminant__7.c`, `expressions/test_conditional_mixed_width__1.c`) are deferred until that fix lands. No tier-3 manifest entries were removed under the unexpected-failure rule. Linux `.asm` byte-identity against Stage 182 was not checked. macOS ARM64 self-host C0→C1→C2 passes (each 2174/2174). Linux x86-64 self-host is pending.
