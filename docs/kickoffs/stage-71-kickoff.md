# Stage 71 – Block-scope Static Support

## Summary of the spec

Add support for `static` storage class in block scope, allowing local static variables with:

- **Block scope**: Variables are visible only within their scope (like auto variables)
- **Static storage duration**: Values persist across function calls (unlike auto variables)
- **Implicit zero-initialization**: Uninitialized statics are placed in `.bss` and start at zero
- **Constant initialization**: Initialized statics must use constant values (int/char literals, unary minus + int literal) and are placed in `.data`

This contrasts with the current behavior where any storage class specifier at block scope (including `static`) produces an error: "error: storage class specifier not allowed in block scope".

Key features:
- Each function's local statics get unique compiler-generated labels (e.g., `Lstatic_f_0`)
- Block/scope shadowing rules still apply (inner `static int x` shadows outer `static int x`)
- Nested blocks can have statics
- Invalid to initialize statics with non-constant values

## Required tokenizer changes

None. TOKEN_STATIC already exists and is properly recognized.

## Required parser changes

Modify `parse_declaration()` in `src/parser.c` to allow `static` at block scope:

1. When parsing a declaration in a block scope and encountering TOKEN_STATIC, accept it (do not error)
2. Consume the TOKEN_STATIC token and recursively call `parse_declaration()` to parse the rest
3. Mark the resulting declaration AST node with `storage_class = SC_STATIC`
4. Leave the error for other storage classes like `extern` at block scope

## Required AST changes

**AST Declaration node changes** (`include/ast.h`):
- Add `storage_class_t storage_class` field to `ASTDeclaration` (or use existing field if present)
- Valid values: `SC_AUTO`, `SC_STATIC`, `SC_EXTERN`, `SC_REGISTER`

**LocalVar struct** (`include/codegen.h`):
- Add `bool is_static` field to track if this is a local static
- Add `char static_label[256]` field to store the generated label for static storage

**CodeGen struct** (`include/codegen.h`):
- Add `LocalStaticVar` struct to represent a local static variable for deferred emission:
  - `char *name`
  - `char label[256]`
  - `Type *type`
  - `bool is_initialized`
  - `int init_value` (for constant initializers)
- Add `LocalStaticVar *local_statics` pool to CodeGen
- Add `int local_static_count` to track number of local statics

## Required code generation changes

**Validation** (`src/codegen.c`):
1. In `codegen_declaration()`, when handling `SC_STATIC` at block scope:
   - Validate that initializers (if present) are constant expressions
   - Reject non-constant initializers with an error
   - Only allow scalar types (int, char, long long, pointers) — no arrays, no structs

**Label generation**:
2. For each local static, generate a unique label: `Lstatic_<function_name>_<N>`
   - Use a counter per function to ensure uniqueness across nested blocks

**Stack and storage handling**:
3. Register local static in the LocalVar array with `is_static = true` and the generated label
4. Do NOT allocate stack space for local statics (they use static storage, not stack)
5. When generating code that references a local static variable, use `[rel label]` (position-independent) addressing instead of `[rbp - offset]`

**Deferred emission**:
6. Collect all local statics into the emission pool during codegen
7. After emitting the function body, emit all collected local statics:
   - Uninitialized statics go to `.bss` section: `label: resd 1` (for int/long long)
   - Initialized statics go to `.data` section: `label: dd <value>` or `dq <value>`

**Scoping and shadowing**:
8. Local statics follow block scoping rules — when a new block is entered, push/pop scopes on the LocalVar symbol table as normal

## Test plan

1. **Valid tests** (add 4 new integration tests):
   - Basic persistence: `static int x;` incremented and returned across calls (expect 3)
   - Explicit initializer: `static int x = 40;` incremented and returned across calls (expect 83)
   - Separate functions: Two functions with their own `static int x` variables don't interfere
   - Nested blocks: `static int x;` inside an `if` block persists across calls

2. **Invalid tests** (modify and add):
   - Update existing invalid test 118: Change `static int x = 1;` to `extern int x;` since static is now valid at block scope
   - Add new invalid test: Non-constant initializer `static int x = y;` where `y` is a local variable (should error)

3. **Run full test suite**: Verify no regressions and all new tests pass

## Implementation order

1. `include/ast.h` – Verify/add storage_class field to ASTDeclaration (if not present)
2. `include/codegen.h` – Add LocalStaticVar struct, is_static and static_label to LocalVar, and local_statics pool to CodeGen
3. `src/parser.c` – Modify `parse_declaration()` to accept and mark TOKEN_STATIC at block scope
4. `src/codegen.c` – Implement validation, label generation, stack handling (skip allocation), and deferred emission
5. Update `test/invalid/118_storage_class_specifier_not_allowed_in_block_scope.c` to use `extern` instead of `static`
6. Add 4 new valid integration tests for local statics
7. Add 1 new invalid integration test for non-constant initializer
8. Build and verify no build errors
9. Run test suite and validate all tests pass

## Known ambiguities and decisions

1. **Typo in spec**: Line 20 has `staiic` which should be `static`. Treating this as a documentation typo, not a language feature.

2. **Label prefix choice**: The NASM `.L` prefix for labels creates scoping issues in data sections. Using `Lstatic_` (no dot prefix) instead to avoid confusion and comply with NASM data section conventions.

3. **Invalid test update**: Existing invalid test 118 uses `static int x = 1;` at block scope. Since this is now valid, the test will be updated to use `extern int x;` instead (which should still produce an error for block scope).

4. **Out of scope features**: Arrays, structs, aggregate initializers, and compound literals are explicitly out of scope per the spec. Only scalar types allowed.

5. **Constant initializer validation**: Only int literals, char literals, and unary minus applied to int literals are accepted. Other constant expressions (e.g., string literals for pointers) are out of scope for this stage.

6. **No AST node position tracking**: Unlike stage 70.03, this stage does not require position information in AST nodes. Position tracking (if needed for error reporting) will rely on existing error context or can be deferred to a future stage.

7. **Block scope shadowing**: Local statics participate in normal block scope shadowing — an inner `static int x` shadows an outer `static int x`, and both persist independently.
