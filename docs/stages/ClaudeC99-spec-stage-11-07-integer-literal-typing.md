# Claude C99 Stage 11-07

## Task
- Add basic type handling for integer literals.

## Requirements
- Integer literal AST nodes must carry a type.
- Unsuffixed decimal literals default to `int` when they fit.
- Unsuffixed decimal literals become `long` when they exceed `int` range but fit in `long`.
- Add support for `L` / `l` suffix for long literals.
- Literal codegen should emit values using the literal’s type.
- Expression typing should use the literal type during promotions/common-type selection.

## Examples
- `123` → `int`
- `123L` → `long`
- `123l` → `long`
- `2147483647` → `int`
- `2147483648` → `long`

## Out of Scope
- unsigned suffixes
- hex/octal/binary literals
- `long long`
- overflow diagnostics beyond "too large for supported integer types"
