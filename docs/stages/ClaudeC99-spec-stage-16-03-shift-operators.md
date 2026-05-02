# ClaudeC99 Stage 16-03

## Task
  - Add support for shift operators `>>` and `<<`
    ```C
      a << b
      a >> b
  - ```

## Token Update
Add tokens for
   `>>`   ->   `TOKEN_RIGHT_SHIFT`
   `<<`   ->   `TOKEN_LEFT_SHIFT`

## Grammar Update

```ebnf
    <relational_expression> ::= <shift_expression> { ( "<" | "<=" | ">" | ">=") <shift_expression> }
    
    <shift_expression> ::= <additive_expression> { ( "<<" | ">>" ) <additive_expression> }

```   

## Requirements
  - The `>>` and `<<` can be applied to `<integer_type>`s only
    - `char`
    - `short`
    - `int`
    - `long`
  - Pointer and Array types are invalid operand types for `>>` and `<<`
    - `int *p; p >> 1; // INVALID`
    - `int *p; p << 1; // INVALID`
    - `int A[10]; A << 1; // INVALID`
    - `int A[10]; A >> 1; // INVALID`
  - normal integer promotion rules are first applied
    - `char`  promoted to `int`
    - `short` promoted to `int`
    - `int`  remains `int`
    - `long` remains `long`
  - for the right shift operator `>>` 
    - given
    - ` n >> m `
    - After promoting `n` the bits are right shifted by `m` positions
    - The resulting type is the same as the promoted type of the left operand `n`
- for the left shift operator `<<`
    - given
    - ` n << m `
    - After promoting `n` the bits are left shifted by `m` positions
    - The resulting type is the same as the promoted type of the left operand `n`
- Tokenizer should match `<<` before `<` and `>>` before `>`
- Both operands must be integer types

## Codegen

For 32-bit `int`
```asm

  ; for promoted left operand of type `int` use

  ; left shift
  ; lhs << rhs
  ; evaluate rhs into eax or another scratch register
  mov ecx, eax
  
  ; evaluate or restore lhs into eax
  ;;;
  ; for left shift use (shl)
  shl eax, cl
  
  ;;;
  ; for signed right shift use arithmetic shift (sar) 
  sar eax, cl
```

for 64-bit 'long'
```asm
  ; lhs 
  
  ; for left operand of type `long` use rax/rcx instead of eax/ecx 
  ; move rhs into rax
  ; then
  mov rcx, rax
  
  ;; evaluate or restore lhs into rax
  ; then use shl to left shitt
  shl rax, cl
  
```

## Out of Scope
  - bad shift counts
    `a << -1`
    `a << 32`

## Tests
Basic Left shift
```C
    int main() {
        return 1 << 3;    // expect 8
    }
```

```C
    int main() {
        return 3 << 2;    // expect 12
    }
```

Basic Right Shift
```C
    int main() {
        return 16 >> 2;   // expect 4
    }
```

```C
    int main() {
        return 9 >> 1;   // expect 4
    }
```

Arithmetic right shift
```C
    int main() {
        return -8 >> 1;   // expect -4, [exit code 252]
    }
```

Variables
```C
    int main() {
        int a = 1;
        int b = 3;
        return a << b;    // expect 8
    }
    
    int main() {
        int a = 16;
        int b = 2;
        return a >> b;   // expect 4
    }
```

Precedence with additive expressions (shifts are lower precedence)
```C
    int main() {
        return 1 + 2 << 3  // expect 24 
    }    
```

Precedence with relational expressions
```C
   int main() {
       return 1 << 3 > 4; // expect 1
   }
```

Left Associativity
```C
    int main() {
        return 32 >> 2 >> 1; // expect 4
    }
```

Char promotion
```C
    int main() {
        char a = 1;
        return a << 2; // expect 4
    }
```

```C
    int main() {
        char a = 8;
        return a >> 1;  // expect 4
```

Short promotion
```C
    int main() {
        short a = 2;
        return a << 3;   // expect 16
    }
```

Long operands
```C
    long foo() {
        long a = 8L;
        return a << 2;
    }
    
    int main() {
        return foo();   // expect 32
    } 
```

```C
    long foo() {
        long a = 32L;
        return a >>> 3;
    }
    
    int main() {
        return foo();   // expect 4
    } 
```

### Invalid Tests
```C
    int main() {
        int a = 1;
        int *p = &a;
        return p << 1;  //INVALID
    }
```

```C
    int main() {
        int a = 1;
        int *p = &a;
        return p >> 2;  // INVALID
    }
```

```C
    int main() {
        int A[10];
        return A << 1;   //INVALID
    }
```

```C
    int main() {
        int A[10];
        return A >> 1;   // INVALID
    }
```

```C
    int main() {
        return 1 <<;    // INVALID
    }
```

```C 
    int main() {
        return 1 >>;  // INVALID
    }
```

```C
    int main() {
        return >> 1;   // INVALID
    }    
```

```C
    int main() {
        return << 1;  // INVALID
    }
```

 