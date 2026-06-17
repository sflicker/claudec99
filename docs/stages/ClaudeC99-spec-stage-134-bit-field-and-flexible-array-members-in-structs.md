#ClaudeC99 stage 134 - bit field and flexible array members in structs

This report lists two current cc99 C99 struct-member bugs found by compiling
and running standalone programs with:

```text
/home/scott/code/claude/claudec99/bin/cc99
```

The reduced examples below were also compiled with GCC using
`gcc -std=c99 -O0 -Wall -Wextra -pedantic`. GCC accepted both programs and
returned the expected values.

## Summary

| ID | Area | Symptom | Priority |
|----|------|---------|----------|
| CC99-006 | Parser / struct declarators | Bit-field members in structs are rejected at the `:` token | Medium |
| CC99-007 | Parser / struct declarators | Flexible array members are rejected as unsized arrays | Medium |

## CC99-006: Bit-Field Members In Structs Are Rejected

### Reduced Source

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

    return (int)(flags.low + flags.mid + flags.high);
}
```

### Compile Commands

```sh
/home/scott/code/claude/claudec99/bin/cc99 -o /tmp/test_bit_field_members_cc99 cc99_testing/test_bit_field_members.c
gcc -std=c99 -O0 -Wall -Wextra -pedantic -o /tmp/test_bit_field_members_gcc cc99_testing/test_bit_field_members.c
```

### Observed Behavior

GCC accepts the program and returns `23`.

cc99 rejects the first bit-field width:

```text
/home/scott/code/c/cc99_benchmarks_and_testing/cc99_testing/test_bit_field_members.c:2:22: error: expected ';', got ':' (':')
```

### Expected Behavior

C99 permits integer bit-field members in structs. cc99 should parse the
bit-field width, allocate the fields within the struct object, allow assignment
through normal member access, and return `23` for this reduced program.

### Likely Fix Area

Struct member declarator parsing currently appears to require each member to be
a normal declarator ending at `;`. Add support for the C bit-field declarator
form:

```c
specifier-qualifier-list declarator_opt : constant-expression ;
```

The implementation also needs object layout and member access handling for
bit-field storage units, including read-modify-write updates when assigning a
single field.

### Regression Tests To Add

- Unsigned bit-field assignment and readback
- Adjacent bit-fields sharing a storage unit
- Anonymous padding bit-fields
- Zero-width bit-fields forcing a new allocation unit
- Invalid address-of bit-field expressions, if semantic diagnostics are in
  scope

## CC99-007: Flexible Array Members In Structs Are Rejected

### Reduced Source

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
}
```

### Compile Commands

```sh
/home/scott/code/claude/claudec99/bin/cc99 -o /tmp/test_flexible_array_members_cc99 cc99_testing/test_flexible_array_members.c
gcc -std=c99 -O0 -Wall -Wextra -pedantic -o /tmp/test_flexible_array_members_gcc cc99_testing/test_flexible_array_members.c
```

### Observed Behavior

GCC accepts the program and returns `34`.

cc99 rejects the flexible array member declaration:

```text
/home/scott/code/c/cc99_benchmarks_and_testing/cc99_testing/test_flexible_array_members.c:5:25: error: struct array member 'data' requires explicit size
```

### Expected Behavior

C99 permits a struct with more than one named member to end with an incomplete
array member. `sizeof(struct Packet)` should exclude the flexible array storage,
while member access through a suitably allocated object should work. The reduced
program should compile and return `34`.

### Likely Fix Area

The struct member parser and type checker appear to treat every array member as
requiring an explicit bound. Add the C99 flexible array member exception for the
last named member of an otherwise non-empty struct, then make struct layout
record the member offset without adding trailing array storage to `sizeof`.

### Regression Tests To Add

- Flexible array member as the last member of a non-empty struct
- `sizeof(struct S)` excluding flexible array storage
- Indexed access through `p->flex[i]`
- Diagnostic for a flexible array member that is not last
- Diagnostic for a struct containing only a flexible array member
