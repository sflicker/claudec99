# Milestone Summary

## stage-52-03: Basic `#if` Constant Conditionals - Complete

stage-52-03 ships limited `#if` support for integer constant conditions, where `0` evaluates to false and nonzero evaluates to true.

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Preprocessor: Added `#if <integer-literal>` directive handler in `preprocess_internal`. Parses integer constant, evaluates to boolean, and controls region emission; inactive regions skip validation. Supports nesting with existing `#ifdef`/`#ifndef` frame stack.
- Tests: 6 new tests (3 valid, 3 invalid). Full suite 934/934 pass.
- Docs: Updated grammar.md to add `<if_constant_directive>` rule; updated README.md to document `#if` constant support and change stage marker to 52-03.
- Notes: Out of scope: `#elif`, `defined()`, macro expansion in `#if`, and arithmetic/comparison/logical expressions in `#if`.
