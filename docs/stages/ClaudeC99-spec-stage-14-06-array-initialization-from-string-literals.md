# ClaudeC99 Stage 14-06

## Task
  - Support initializing local `char`arrays from string literals.

## Requirements
  - Support explicit-size char array initialization
     `char s[3] = "hi";`
  - Support inferred-size char array initialization
     `char s[] = "hi";`
  - for inferred-size arrays, set the array length to the string literal byte length plus one for the null terminator.
  - Copy the string literal contents into the local stack array.
  - include the terminating null byte
  - Existing tests must continue to pass.

## Type Rules
A string literal has length.
  `literal_byte_length + 1`
where the extra byte is the null terminator

Examples
```C
   "hi"            // 3 bytes: 105, 106, 0
   ""              // 1 bytes: 0
   "A\0B"          // 4 bytes: 65, 0, 66, 0
```

### Explicit size rules
` char[3] = "hi"; `

This is also valid:
` char s[5] = "hi";`
The remaining elements should be zero filled

This is invalid:
` char s[2] = "hi"`
because "hi" needs 3 bytes including the null terminator.

### Inferred size rules
This:
` char s[] = "hi"; `
should behave as:
` char [3] = "hi"; `

This:
` char s[] = ""; `
should behave as:
` char[1] = "";`

This:
` char s[] = A\0B";`
should infer size 4.

## Grammar Update
```ebnf
<declaration> ::= <type> <identifier> [ "[" [ <integer_literal> ] "]" ] [ "=" <expression> ] ";"
```

Restriction:
An Omitted array size is only allowed when the declaration has a string literal initializer.

## Codegen
For
` char s[3] = "hi"; `
The compiler should allocate stack storage for 3 bytes and initialize.
```C
  s[0] = 104
  s[1] = 105
  s[2] = 0
```

For
` char s[5] = "hi";`
initialize:
```C
  s[0] = 104
  s[1] = 105
  s[2] = 0
  s[3] = 0
  s[4] = 0
```

## Tests

### Valid Tests
```C

    int main() {
       char s[] = "hi";
       return s[0];    // expect 104
    }
    
    int main() {
        char s[] = "hi";
        return s[1];  // expect 105
    }
    
    int main() {
        char s[] = "hi";
        return s[2];  // expect 0
    }
    
    int main() {
        char[5] = "hi";
        return s[4];    // expect 0
    }
    
    int main() {
        char s[] = "A\n";
        return s[1];     // expect 10
    }
    
    int main() {
        char s[] = "A\0B";
        return s[2];   // expect 66
    }
```

### Invalid Tests
```C
    int main() {
        char s[2] = "hi";
        return 0;     // Expected Error: array too small for string literal initializer.
    }
    
    int main() {
        int s[] = "hi";
        return 0;       // expected Error: string initializer only supported for char arrays.
    }
    
    int main() {
        char s[];
        return 0;       // Expected Error: array size required unless initialized from string literal.
    }
    
    int main() {
        char s[] = 123;
        return 0;           // Expected Error: omitted array size requires string literal initializer.
    }
```

### Out of Scope
  - General initializer lists
  - Non-char array initialization from string literals
  - Global arrays
  - Multi-Dimensional arrays
  - Adjacent string literal concatenation
  - character literals
