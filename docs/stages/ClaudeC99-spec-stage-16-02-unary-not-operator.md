# ClaudeC99 Stage 16-02

## Task
  - Add support for the unary bitwise complement `~` operator
  - Example ` ~a `

## Token Update
ADD token for `~` -> `TOKEN_TILDE`

## Grammar Update
```ebnf
    <unary_operator> ::= "+" | "-" | "!" | "++" | "--" | "*" | "&" | "~"
```

## Requirements
  - The `~` can be applied to `<integer_type>`s only. 
    - `char`
    - `short`
    - `int`
    - `long`
  - Pointer and Array types are invalid operand types for `~`
    - `int *p; ~p; // INVALID`
    - `int A[10]; ~A; // INVALID`
  - The result of the `~` is the bitwise complement of its (promoted) operand.
    - Each bit in the result is set if and only if the corresponding bit in the converted operand was not set.
    - Integer promotions are performed on the operand and the result has the promoted type.
    - Example 
        ``` // given 
              char a = 2; 
              ~a;  // result type is int
            // than
              a -> promoted to int -> bitwise complement of int -> result type int
              char value 2
              00000010
      
              promoted to int 
              00000000 00000000 00000000 00000010
      
              complement
              11111111 11111111 11111111 11111101
      
        ```
  - start type   apply ~   result type
    ` char                    int`
    ` short                   int`
    ` int                     int`
    ` long                    long`

## Out-of-scope
  - unsigned operands.

## Codegen
  - for `int` sized promoted values:
```asm 
      not eax 
  ```

  - for `long` sized operands
```asm
       not rax
```
  - for `char` and `short`, the operand should be promoted to `int` before applying complement

## Tests
**BASIC CASES**
```C
    int main() {
        return ~2;  // expect -3 [Exit code to verify is 253]
    }
``` 
```C
    int main() {
        return ~0;  // expect -1,[exit code to verify is 255]
    }
```    
```C    
    int main() {
        return ~-1;  // expect 0
    }
```
    
**NESTED COMPLEMENT**
```C
    int main() {
        return ~~5;    // expect 5
    }
```    
    
**VARIABLES**
```C 
    int main() {
       int a = 2;
       return ~a;    // expect -3 [exit code 253]
    }
```    
    
**char promotion**
```C
    int main() {
        char a = 2;
        return ~a;   // expect -3 [exit code 253]
    }
```    
    
**short promotion**
```C    int main() {
        short a = 2;
        return ~a;  // expect -3 [exit code 253]
    }
```

**long operand**
```C    
    long f() {
        long a = 2L;
        return ~a;
    }
    
    int main() {
        return f();  // expect -3 [exit code 253]
    }
```    

** Precedence** 
```C
    int main() {
        return ~2 + 4;   // expect 1;
    }
```


## Invalid Tests
```C
    int main() { int a=2; int *p = &a; return ~p; }  // INVALID
    int main() { int A[2]; return ~A; } // INVALID 
```