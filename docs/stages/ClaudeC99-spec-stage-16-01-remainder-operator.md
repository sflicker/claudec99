# ClaudeC99 Stage 16-01

## Task 
  - Add the remainder operator
    example ' a % b '

### Token Update
```
    TOKEN_PERCENT     for the % character
```

## Grammar Updates
```ebnf
    <multiplicative_expression> ::= <cast_expression> { ("*" | "/" | "%" ) <cast_expression> }
```
This operator is left-associative, like `*` and `/` 

### Parser and AST
The remainder operator will be stored as a BINARY_OP in an AST Node.

### Semantics
The remainder operator can only be used with integer types
Allowed operand types should follow the compiler's current rules for:
```C
    char
    short
    int
    long
```

Pointer Types are invalid operands
Array values are also invalid as direct operands. 

Division by zero / remainder by zero does not need a check in this stage.

### Codegen
Codegen notes: for signed integers on x86-64 this uses `idiv`, but
the result comes from the remainder register:

For 32 bit `int` sized operations
```asm
    ; eax = left operand
    ; rhs = right operand in register or memory
    
    cdq
    idiv rhs
    
    // quotient is now in eax
    // remainer is now in edx
    
    mov eax, edx
```

For 64 bit `long` operators
```asm
    ;rax = left operand
    ;rhs = right operand is in register or memory
    
    cqo
    idiv rhs
    
    ; quotient is now in rax
    ; remainder is now in rdx

    mov rax, rdx
```
The generated code should preserve the existing evaluation order used by the other binary operations

Important `idiv` cannot divide directly by an immediate operand, so if the right side
is a literal or computed expression, it should be placed into a register or temporary location before idiv.


### Tests
```C
    // basic remainer tests
    
    int main() {
        return 1 % 1;    // expect 0
    }
    
    int main() {
        return 5 % 4;    // expect 1
    }
    
    int main() {
        return 20 % 6;    // expect 2
    }
    
    // precedences with multiplication and division
    int main() {
        return 1 + 2 % 2 * 6;   // expect 1
    }
    
    int main() {
        return 1 + 5 % 3 / 2;   // expect 2
    }
    
    // left associativity
    int main() {
        return 2 % 4 % 2;   // expect 2
    }
    
    // variables
    int main() {
       int a = 5;
       int b = 4;
       return a % b;   // expect 1
    
    // in conditionals
    // expect 1
    int main() {
        int a = 4;
        int b = 2;
        if (a % b) {
            return 2;
        }
        else {
            return 1;
        }
    }
    
    // int loops
    // expect 5
    int main() {
        int a = 7;
        int b = 2;
        int rtn = 0;
        while(a % b) {
            rtn++;
            b++;
        }
        return rtn;
    }
    
    // long values
    // expect 2
    int main() {
        long a = 20L;
        long b = 6L;
        return a % b;      // return 2
```

Invalid Cases:
```C
    int main() { return %; }
    int main() { return 1%; }
    int main() { return %1; }
    int main() {
        int * p;
        int b = 1;
        return p % b;
    }
    
    int main() {
        int A[8];
        int b=1;
        return A % b;
    } 
```
