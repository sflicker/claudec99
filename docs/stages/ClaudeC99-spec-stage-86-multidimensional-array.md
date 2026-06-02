# ClaudeC99 Stage 86 multidemsional array support

## Task
  - Support multidemsional arrays both free standing and as members of structs
  - support declarations
  - member access and indexing
  - both rvalue and lvalue use
  - dimeneions higher than two should also be supported but a reasonable upper limit of dimensions maybe used.
  Examples
 ```C
  int main() {
    int A[2][2];
    A[0][0] = 1;
    A[0][1] = 2;
    A[1][0] = 3;
    A[1][1] = 4;

    return A[0][0] + A[0][1] + A[1][0] + A[1][1]; // expect 10
  }
```

```C
struct S {
    int table[4][8];
};

int main() {
    struct S s;
    s.table[0][0] = 42;
    return s.table[0][0];  // expect 42
}
```

## Grammar
The existing grammar should support multi dimension arrays

## Semantic and type support
A declarator may contain multiple array suffixs. For a member declaration
`int table[4][8];`, the resulting type is array[4] of array[8] of int.

## sizeof rules
given `int table[4][8]`
`sizeof(table[0]) = 8 * sizeof(int) = 32`
`sizeof(table) = 4 * 8 * sizeof(int) = 128`
`sizeof(int[8]) = 32`
`sizeof(int[4][8]) = 128`

## struct member layout
struct layout should treat a multidimensional array member like any other member with a computed size and alignment.

Example
```C
struct S {
    char tag;
    int table[4][8];
};
```
Expected layout on current target:
```text
tag offset        = 0
padding           = 3 bytes
table offset      = 4
table size        = 4 * 8 * 4 = 128 bytes
struct size       = 132, rounded to max alignment 4 => 132
struct align      = 4
```

## indexing behavior
given:
```C int table[4][8]; ```
for:
```C table[i][j] ```
The expression should work as;
```text
table        : array[4] of array[8] of int
table[i]     : array[8] of int lvalue, decays to pointer where needed
table[i][j]  : int lvalue

## Address Calculation
```C
for:  int table[4][8]
address(table[i][j]) = address(table) + i * sizeof(int[8]) + j * sizeof(int)
```

```C
for:  struct S { int table[4][8]; }; struct S s;
address(s.table[i][j]) = address(s) + offset(table) + i * sizeof(int[8]) + j * sizeof(int)
```

for pointer access to member: `p->table[i][j]
```C
value(p) + offset(table) + i * sizeof(int[8]) + j * sizeof(int)
```

## Array-to-pointer decay interaction
given: `struct S { int table[4][8]; }; struct S s;`
Then
`s.table[i]` 
Its type is array of int, but in most expression contexts it decays to int *.
But
`s.table[i][j]`
The first subscript should still work because it is equivalent to:
```C 
*(*(s.table + i) + j)
```
so after the first index, the result is an array element that can decay for the second subscript.

This stage should make sure the existing postfix chaining and member-array decay behavior
also work for nested array types.

## Codegen
The lvalue-address generator should recursively handle array element types.

for a subscript expression:
``` C
base[index]
```

do
```text
base_addr   = address/value of base after array decay rules
scale       = sizeof(element_type)
addr        = base_addr + index * scale
```

For multidimensional arrays, the first subscript's element type may itself by an array.
Example:
```C
s.table[i]
```
where:
```text
table type  = array[4] of array[8] of int
element type = array[8] of int
scale = sizeof(array[8] of int) = 32

THen:
```C
s.table[i][j]
```

second subscript;
```C
bsae type     = array[8] of int
elemnent type = int
scale = 4
```

## Implementation Notes
Declarator constructor order
For:
```C
int table[4][8];
```
make sure the built type is:
`array[4] of array[8] of int`    // CORRECT
and not
`array[8] of array[4] of int`    // INCORRECT

## Tests
Basic test
```C
int main() {
    int table[1][1];
    table[0][0] = 42;
    return table[0][0];   // expect 42    
}
```

2,2 array test
```C
  int main() {
    int A[2][2];
    A[0][0] = 1;
    A[0][1] = 2;
    A[1][0] = 3;
    A[1][1] = 4;

    return A[0][0] + A[0][1] + A[1][0] + A[1][1]; // expect 10
  }
```

Array as struct member test
```C
struct S {
    int table[4][8];
};

int main() {
    struct S s;
    s.table[0][0] = 42;
    return s.table[0][0];  // expect 42
}
```

sizeof test
```C
----- main test .f ----------
#include <stdio.h>

int main() {
    printf("%ld ", sizeof(int[1]));
    printf("%ld ", sizeof(int[4]));
    printf("%ld ", sizeof(int[2][2]));
    printf("%ld\n", sizeof(int[4][8]));
}
------------------------------

-------- .expected ------------
4 16 16 128
-------------------------------
```

```C
struct S {
    int table[4][8];
};

int main(void) {
    struct S s;
    return sizeof(s.table[0]) == 32;   // expect 1
}
```

Pointer to struct access
```C
struct S {
    int table[4][8];
}

int main() {
    struct S s;
    struct S *p;

    p = &s;
    p->table[2][3] = 42;

    return p->table[2][3];   // expect 42
}
```

Dynamic indices
```C
struct S {
    int table[4][8];
};

int main(void) {
    struct S s;
    int i = 3;
    int j = 5;

    s.table[i][j] = 42;
    return s.table[3][5];    // expect 42 

}
```

Char 2D array
```C
int main() {
    char names[2][4];

    names[0][0] = 'O';
    names[0][1] = 'K';
    names[0][2] = '\n';

    // 'O' + 'K' = 79 + 75 = 154, 154 -112 = 42
    return names[0][0] + names[0][1] - 112;  //expect 42
}
```

struct layout with preceding field
```C
struct S {
    char tag;
    int table[4][8];
};

int main() {
    struct S s;

    s.tag = 1;
    s.table[0][0] = 41;

    return s.tag + s.table[0][0];    // expect  42;
}
```

higher dimension test
```C
int main() {
    int table[2][2][2];

    table[0][1][0] = 42;

    return table[0][1][0];   // expect 42
}
```

```C
------- main test .c --------
#include <stdio.h>
/* 
    sizeof(int[4][8][16])
  = 4 * sizeof(int[8][16])
  = 4 * 8 * sizeof(int[16])
  = 4 * 8 * 16 * sizeof(int)
  = 4 * 8 * 16 * 4
  = 2048
  */
int main() {
    printf("%ld\n", sizeof(int[4][8][16]));
}
-----------------------------

------ .expected ------------
2048
-----------------------------
```

built order test
```C
struct S {
    int table[4][8];
};

int main() {
    struct S s;
    return sizeof(s.table[0]);   // expect 32
}
```

## Invalid tests
too many subscripts
```C
struct S {
    int table[4][8];
};

int main(void) {
    struct S s;

    return s.table[0][0][0];     // INVALID
}
```

Non integer size
```C
struct S {
    int n;
    table[4][n];      // invalid
};

int main() {
    return 0;
}
```

Missing inner size (not currently supported)
```C
struct S {
    int table[4][];    // invalid
};
```

