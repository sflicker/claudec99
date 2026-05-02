# ClaudeC99 Stage 16-04

## Task
  - add bitwise binary operators
    - `&` bitwise and
    - `^` bitwise XOR
    - `|` bitwise OR

## Important Distinction
  - These operators share characters with others so care must be taken to use the correct one.
```C
    &p       // unary address of
    a & b    // binary bitwise and
    
    a && b   // logical and
    a & b    // bitwise and
    
    a || b   // logical or
    a | b    // bitwise or
  ```

## Token Updates
```C
   TOKEN_CARET   // ^
   TOKEN_PIPE    // |
```

## Grammar Updates

```ebnf
<logical_and_expression> ::= <bitwise_or_expression> { "&&" <bitwise_or_expression> }

<bitwise_or_expression> ::= <bitwise_xor_expression> { "|" <bitwise_xor_expression> }

<bitwise_xor_expression> ::= <bitwise_and_expression> { "^" <bitwise_and_expression> }

<bitwise_and_expression> ::= <equality_expression> { "&" <equality_expression> }
```

## Important Precedence examples
```C
   1 & 3 == 3    Parses as 1 & (3 == 3)  -> 1 & 1 -> 1
   1 | 2 & 4     Parses as 1 | (2 & 4) -> 1 | 0 -> 1
   1 ^ 3 & 1     Parses as 1 ^ (3 & 1) -> 1 ^ 1 -> 0
```

## Semantics
Operands must be integer types only:
```C
   char
   short
   int 
   integer
```

Invalid
```C
    int *p;
    p & 1;  // invalid
    
    int a[10];
    p | 1;   // invalid
```

Operands are promoted before apply operator 
```C
   char  -> int
   short -> int
   int   -> int
   long  -> long
   
   for 
   <int> & <long>   the <int> would first be promoted to <long> 
```

## Codegen
For 32-bit  `int` size results:

```asm             ;lhs must be evaluted into eax
  and eax, rhs     ;rhs must be evaluated into a register
  xor eax, rhs
  or  eax, rhs
```

For 64-bit  `long` size results

```asm
    and rax, rhs     // lhs must evaluted into rax
    xor rax, rhs     // rhs must be evaluted into a register
    or  rax, rhs

```

## Tests
Basic
```C
    int main() {
        return 6 & 3;    //expect 2
    }
```

```C
    int main() {
        return 6 | 3;    // expect 7
    }
```

```C
    int main() {
        return 6 ^ 3;    // expect 5
    }
```

Variables
```C
    int main() {
        int a = 6;
        int b = 3;
        return a & b;    // expect 2
    }
```

Precedence
```C
    int main() {
        return 1 | 2 & 4;  // expect 1
    }
```

```C
    int main() {
        return 1 ^ 3 & 1;   // expect 0
    }
```

```C
    int main() {
        return 1 & 3 == 3; // expect 1
    }
```

Associativity
```C
   int main() {
       return 7 & 3 & 1;  // expect 1
   }
```

Mixed logic operators
```C
    int main() {
        reutrn 0 | 2 && 3;    //expect 1
    }
```

INVALID:
```C
    int main() {
        int a = 1;
        int *p = ^&a;
        return p & 1;  //INVALID
    }
```

```C
    int main() {
        int A[10];
        return A | 1;   // INVALID
    }
```