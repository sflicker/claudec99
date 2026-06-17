# Stage 134 Kickoff — Bit-Field and Flexible Array Members in Structs

## Overview

Stage 134 fixes two struct-member parsing bugs found by compiling standalone
programs with the cc99 compiler:

1. **CC99-006**: Bit-field members in structs are rejected at the `:` token
2. **CC99-007**: Flexible array members are rejected as unsized arrays

Both are C99 features that gcc accepts and produces correct runtime behavior for.
This stage adds parser support, AST representation, and code generation handling
for both features.

## Spec Summary

### Bug CC99-006: Bit-Field Members in Structs

**Symptom**: cc99 rejects the `:` token after a struct member declarator.

**Required behavior**: Parse bit-field width, allocate fields within the struct
object using bit-packing, allow assignment through normal member access, and
handle read/write access through shift+mask operations.

**Test case**:
```c
struct Flags {
    unsigned int low : 3;
    unsigned int mid : 5;
    unsigned int high : 10;
};
int main(void) {
    struct Flags flags;
    flags.low = 5;
    flags.mid = 17;
    flags.high = 513;
    return (int)(flags.low + flags.mid + flags.high);  // expect 35
}
```

GCC compiles and returns `35`. cc99 rejects with "expected ';', got ':'".

### Bug CC99-007: Flexible Array Members in Structs

**Symptom**: cc99 rejects unsized array members with "requires explicit size".

**Required behavior**: Allow a single unsized array as the last named member of
a struct with at least one other named member. `sizeof(struct)` excludes the
flexible array storage. Member access through pointers works via normal array
indexing.

**Test case**:
```c
void *malloc(unsigned long size);

struct Packet {
    int count;
    unsigned char data[];
};
int main(void) {
    struct Packet *packet = malloc(sizeof(struct Packet) + 4UL);
    packet->count = 4;
    packet->data[0] = 3;
    packet->data[1] = 5;
    packet->data[2] = 7;
    packet->data[3] = 11;
    return packet->count + packet->data[0] + packet->data[1]
        + packet->data[2] + packet->data[3] + (int)sizeof(struct Packet);
    // expect 34
}
```

GCC compiles and returns `34`. cc99 rejects with "struct array member 'data'
requires explicit size".

## Required Changes

### 1. Tokenizer Changes

None. The `:` token already exists and is properly recognized.

### 2. Parser Changes (src/parser.c, parse_struct_specifier)

1. Track bit-field packing state during struct member parsing:
   - Current storage unit offset within the struct
   - Current storage unit size (from the bit-field width type)
   - Bits used in the current storage unit

2. After parsing type specifier, check for `:` token:
   - If `:` appears without a declarator (anonymous bit-field):
     - Parse width constant expression
     - Update packing state
     - Continue to next member (no field added to symbol table)
   - If declarator exists and followed by `:` (named bit-field):
     - Parse width constant expression
     - Add field to struct with bit-field metadata
     - Update packing state
   - If no `:` token, handle as normal struct member

3. Special handling for zero-width anonymous bit-fields:
   - Close the current storage unit (advance struct offset)
   - Do not add any bytes to struct size
   - Next bit-field starts in a new storage unit

4. Flexible array member support:
   - Allow `d.is_array && !d.has_size` for the last member only
   - Only if struct has at least one other named field
   - Record the flexible array flag in the struct field

### 3. AST Changes (include/ast.h, StructField)

Add fields to represent bit-field and flexible array metadata:

```c
int is_bitfield;        // Set for bit-field members
int bit_width;          // Width in bits (0 for zero-width padding)
int bit_offset;         // Bit offset within the storage unit (0-based)
int is_flexible_array;  // Set for C99 flexible array members
```

### 4. Code Generation Changes (src/codegen.c)

#### Bit-field Read (rvalue)

When accessing a bit-field member for reading:
1. Load the storage unit (struct base + field offset, field type)
2. Shift right by `bit_offset` bits
3. Mask with `(1 << bit_width) - 1`
4. Extend to int if needed (default argument promotion context)

#### Bit-field Write (lvalue assignment)

When assigning to a bit-field member:
1. Evaluate RHS to a value
2. Mask RHS to `bit_width` bits: `rhs & ((1 << bit_width) - 1)`
3. Shift to position: `(rhs & mask) << bit_offset`
4. Load current storage unit value
5. Clear bits in storage unit: `unit & ~(mask << bit_offset)`
6. OR in new bits: `unit | (shifted_rhs)`
7. Store back to storage unit

#### Flexible Array

No special code generation needed. Existing `TYPE_ARRAY` decay-to-pointer and
array-indexing paths handle member access correctly when the field has
`type_array(elem_type, 0)` with the correct field offset.

### 5. Version Bump (src/version.c)

Change `VERSION_STAGE` from `"13300000"` to `"13400000"`.

## Test Plan

1. **test/valid/structs/test_bit_field_members__23.c**
   - Unsigned int bit-fields in a struct
   - Read and write through member access
   - Expected exit code: `23` (basic arithmetic on bit-field values)

2. **test/valid/structs/test_bit_field_adjacent__NN.c**
   - Multiple adjacent bit-fields sharing one storage unit
   - Verify correct packing and isolation of values
   - Expected exit code: verified calculation

3. **test/valid/structs/test_bit_field_anonymous__NN.c**
   - Anonymous padding bit-fields (no declarator)
   - Verify correct packing behavior
   - Expected exit code: verified calculation

4. **test/valid/structs/test_bit_field_zero_width__NN.c**
   - Zero-width anonymous bit-fields forcing new allocation unit
   - Verify struct layout changes correctly
   - Expected exit code: verified calculation

5. **test/valid/structs/test_flexible_array_member__34.c**
   - Flexible array member in a struct
   - Verify `sizeof(struct)` excludes flexible array
   - Verify indexed access through pointer
   - Expected exit code: `34`

6. **test/invalid/aggregates/test_flexible_array_only_member__1.c**
   - Struct containing only a flexible array member
   - Expected: compilation error (struct must have at least one other field)

7. **test/invalid/aggregates/test_flexible_array_not_last__1.c**
   - Flexible array member not as the last member
   - Expected: compilation error (FAM must be last)

8. Run full test suite to verify no regressions in struct handling, member access,
   and sizeof calculations.

9. Self-host cycle: `./build.sh --mode=self-host`.

## Implementation Order

1. `include/ast.h` — add bit-field and flexible array fields to `StructField`
2. `include/parser.h` — add bit-packing state tracking (if new struct needed)
3. `src/parser.c` — implement bit-field and flexible array parsing in
   `parse_struct_specifier`, add validation for FAM constraints
4. `src/codegen.c` — implement bit-field read/write code generation paths
5. `src/version.c` — version bump to `13400000`
6. New test files (5 valid, 2 invalid regression tests)
7. Full test suite verification
8. Self-host bootstrap
9. Commit with appropriate Co-Authored-By trailer

## Notes & Decisions

- **Bit-field storage units**: The spec uses the bit-field member's type as the
  storage unit type (e.g., `unsigned int`). On AMD64, `unsigned int` is 32 bits.
  Packing respects type boundaries: a new type forces a new storage unit.

- **Flexible array semantics**: FAM is allowed only as the last named member of a
  struct with at least one other named member. This prevents ambiguity in struct
  layout. `sizeof(struct S)` does not include FAM storage.

- **Bit-field address-of**: Taking the address of a bit-field member is undefined
  in C99 and may not be semantically validated in this stage if semantic analysis
  is not in scope. Focus on parsing and code generation.

- **Zero-width bit-fields**: A zero-width anonymous bit-field forces the next
  bit-field to start in a new storage unit. This is used for alignment padding.

- **Integration with existing code**: Struct layout calculations are in
  `src/codegen.c` (computing field offsets). Bit-field packing state must be
  tracked during layout, not during semantic analysis. The parser must compute
  offsets and widths for the codegen phase.

## Ambiguities & Open Questions

1. **Bit-field type requirements**: Are only integer types allowed (int, unsigned
   int, short, etc.)? The spec uses only `unsigned int`. Assume yes; reject
   non-integer types in parser validation.

2. **FAM in unions**: Are unions allowed to contain FAM? The spec only addresses
   structs. Assume unions are not allowed to contain FAM (error if attempted).

3. **FAM with typedef**: If a struct with FAM is typedef'd, does the typedef work
   correctly for declaration and definition? Should work (normal typedef semantics).

4. **Nested FAM**: Can a FAM member have an incomplete (unsized) array type pointing
   to a struct containing FAM? Should be allowed (incomplete arrays are opaque).
