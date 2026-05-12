# Milestone Summary

## Stage 37: Incomplete Struct Declarations - Complete

stage-37 ships support for incomplete struct forms (forward declarations), enabling self-referential struct types via typedef aliases and opaque pointer fields.

- Tokenizer: No changes.
- Grammar/Parser: Enhanced `parse_struct_specifier` to support forward declarations (`struct Tag;`). When a struct body is not yet available, a placeholder type with zero size is created. When the struct body is later defined, the existing placeholder is mutated in-place (size, alignment, fields updated) to ensure typedef aliases remain valid.
- AST/Semantics: Added validation to reject non-pointer incomplete struct fields; pointer-to-incomplete-struct fields are allowed. Typedef entries to incomplete structs now automatically reflect the complete layout when the body is defined.
- Codegen: No changes.
- Tests: 2 new valid tests (self-referential typedef, forward-declared opaque pointer). Invalid test renamed to reflect new error message. Full suite 772/772 pass.
- Docs: Grammar notes and README updated to reflect forward declaration support.
- Notes: In-place placeholder mutation ensures typedef pointers stay valid across forward declaration and body definition. Non-pointer incomplete fields are still rejected as per C semantics.
