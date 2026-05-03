# ClaudeC99 Stage-17-02  sizeof expresions

## Task
  - support sizeof expressions

examples
```C
   sizeof x
   sizeof(x)
   sizeof(a + b)
   sizeof(*p)
   sizeof(A[0])
```

## Grammar Updates
```ebnf
<unary_expression> ::= "sizeof" <unary_expression>
                    | "sizeof" "(" <type> ")"
                    | <unary_operator> <unary_expression>
                    | <postfix_expression>
```

## Requirements
  - sizeof expression determines the expression's type but does not evaluate the expression.
  - example
```C
    int x = 1;
    sizeof(x++);    // should not increment x 
  ```
  - For sizeof(expression), the compiler determines the type of the expression using the normal expression typing rules, including integer promotions and usual arithmetic conversions. The expression is not evaluated, and no runtime code is emitted for evaluating it.
```C
  int main() {
    char a = 1;
    char b = 2;
    return sizeof(a + b);   //sizeof(int) -> expect 4
  }
  
  int main() {
      char a = 1;
      int b = 2;
      return sizeof(a +b);   //sizeof(int) -> expect 4
  }
  
  int main() {
      int a = 1;
      long b = 2;
      return sizeof(a + b);    // sizeof(long) -> expect 8
  ```
The result of sizeof(expression) should be a compile-time constant value.

For now, the result remains the same as Stage-17-01  `long`.

### Expression Behavior
Integer Expressions
```C
    sizeof(1)      // 4
    sizeof(1L)     // 8
    sizeof('a')    // 4
    sizeof(char_var)  // 1 
    sizeof(short_var) // 2
    sizeof(int_var)   // 4
    sizeof(long_var)  // 8
    sizeof(char_var + 1)  // 4   because char promotes to int
```

Pointer Expressions
```C
    int *p
    sizeof(p)     // 8
    sizeof(*p)    // 4
```
Important:  `sizeof(*p)`  must not load from `p`. it only determines the type of `p`.
so this should be valid even though `p` is uninitialized

```C
     int *p;
     return sizeof(*p)   // valid. expect 4
```

Array indexing expressions
This stage support `sizeof(A[0]` because `A[0]` hs the element type.

```C
    int main() {
        int A[0];
        return sizeof(A[0]);    // expect 4
    }
```

However whole-array `sizeof` will be deferred to a later stage.
```C
    int A[1];
    sizeof(A);    // NOT VALID for this stage. 
```
That case is special because array-to-pointer decay must not happen under `sizeof`

### Codegen
For `sizeof(expression)`  do not generate code for the child expression.
Instead
1. Determine the expression type.
2. Compute the size as a constant
3. Emit that size as a constant.
Examples
```asm

   ; sizeof(int expression)
   mov rax, 4
   
   ;; sizeof(long expression)
   mov rax, 8
   
   ; sizeof(*int_pointer);
   mov rax, 4
```
This is especially important for side-effect expressions:
```C
    sizeof(x++);          // do not increment x 
    sizeof(a = 5);        // do not assign 5 to a
    sizeof(function_call())  // do not do the function call   
```
The expression should not be evaluated.

### out of scope
```C
     sizeof(A)       // whole array object size; defer to later stage
     sizeof(int[10]);  
     variable length arrays
     struct
     union
     enum
     unsized
     double
     float
     size_t
```

### Tests
Basic variables

```C
    int main() {
        char x = 1;
        return sizeof(x);   // expect 1
    }
```

```C
    int main() {
        short x = 1;
        return sizeof(x);    // expect 2
    }
```

```C
    int main() {
        int x = 1;
        return sizeof(x);    // expect 4
    }
```

```C
    int main() {
        long x = 1;
        return sizeof(x);   // expect 8
    }
```

Non parenthesized expression form
```C
    int main() {
        int x = 1;
        return sizeof x;    // expect 4
    }
```

```C
    int main() {
        long x = 1;
        return sizeof x;    // expect 8
    }
```

Arithmetic expression typing

```C
    int main() {
        int x = 1;
        char y = 1;
        return sizeof(x + y);    // expect 4
    }
```

```C
    int main() {
        char x = 1;
        char y = 1;
        return sizeof(x+y);    // expect 4
    }
```

```C
    int main() {
        long x = 1;
        int y = 2;
        return sizeof(x+y);    // expect 8
    }
```

Pointer expressions
```C
    int main() {
        int x = 1;
        int *p = &x;
        return sizeof(p);   // expect 8
    }   
```

```C
     int main() {
         int *p;
         return sizeof(*p);   // expect 4
     }
```

```C
    int main() {
        char *p
        return sizeof(*p);    // expect 1;
    }
```

```C
    int main() {
        long *p
        return sizeof(*p);   // expect 8
    }
```

Array element expression
```C
    int main() {
        int A[10];
        return sizeof(A[0]);    // expect 4
    }
```

```C
    int main() {
        char A[10];
        return sizeof(A[0]);    // expect 1
    }
```

Expression is not evaluated
```C
    int main() {
        int x = 1;
        sizeof(x++);
        return x;          // expect 1
    }
```

```C
    int main() {
        int x = 1;
        sizeof(x = 5);
        return x;       // expect 1
    }
```

```C
    int main() {
        int x = 1;
        int *p = &x;
        sizeof(*p = 5);
        return x;     // expect 1
    }
```
Existing Tests should continue to pass


### Invalid tests
Incomplete sizeof
```C
    int main() {
        return sizeof;   // INVALID
    }
```

```C
    int main() {
        return sizeof();   // INVALID
    }
```

Invalid operand expression
```C
    int main() {
        return sizeof(1 +);   // INVALID
    }
```