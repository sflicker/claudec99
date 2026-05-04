# ClaudeC99 stage 18 conditional operator

## Task
  - add support for the conditional operator `?:`
  - `condition` ? `expr_true` : `expr_false`
  - The conditional operator should evaluate the `condition` first.
  - if the `condition` is nonzero, evaluate and return `expr_true`
  - otherwise evaluate and return `expr_false`
  - only one of `expr_true` or `expr_false` expression should be evaluated
Examples
```C
    int main() {
        int a = 1;
        return a ? 10 : 20;  // expect 10
    }   
    
    int main() {
        int a = 0;
        return a ? 10 : 20;  //expect 20
    }
```


## Token Update
add separate tokens (if not already present) for 
`?`  and `:`

## Parser and Grammar Updates
```ebnf
<assignment_expression> ::= <unary_expression> <assignment_operator> <assignment_expression>
                           | <conditional_expression>
                           
<conditional_expression> :: <logical_or_expression>
                          | <logical_or_expression> "?" <expression> ":" <conditional_expression>
```
### Precedence and Associativity
The conditional operator:
  - has lower precedence than logical OR
  - has higher precedence than assignment
  - is right-associative

## Codegen Expectation
For:
```C
    cond ? expr_true : expr_false
```

```asm
    ; evaluate cond (into rax) then
    cmp rax, 0
    je .false_label
    
    ; true branch
    ; evaluate expr_true
    ...
    jmp .end_label
    
    .false_label
    ; evaluate expr_false
    
    .end_label
    ; resule in in rax/eax according to the expression type

```
Labels must be unique, following the existing approach used elsewhere

## Semantics
  - The conditional operator must only evaluate of of the two result expressions.
  - The first operand (`condition`) must be a scalar expression
  - The second and third operands (`expr_true` and `expr_false`) are supported when
    1. both are integer expression
       - using existing integer conversion/promotion rules
       - the result type is the common integer type
    2. both are compatible pointer types
       - The result type is that pointer type
    3. One is a pointer expression and the other is the integer constant `0`
       - The result type is that pointer type
  - The conditional expression is not an lvalue in this stage.

## Out of scope
  - full C99 usual arithmetic conversions involving unsigned/floating types
  - struct/union operands
  - void operands
  - `void *` composite pointer rules
  - conditional expression as an lvalue

## Tests
Basic true case
```C
    int main() {
        return 1 ? 10 : 20;   // expect 10
    }
```
Basic false case
```C
    int main() {
        return 0 ? 10 : 20;   // expect 20
    }
```

Condition from comparison:
```C
    int main() {
        int x = 5;
        return x > 3 ? 1 : 0;  //expect 1
    }
```

Only selected branch is evaluated
```C
    int main() {
        int x = 1;
        int y = 0;
        return x ? 10 : (1/y);   // expect 10  (division by zero in false expression is not evaluated)
    }
```

```C
    int main() {
        int x = 0;
        int y = 0;
        return x : (1/y) : 20;   // expect 20
    }
```

Right associativity
```C
    int main() {
        return 0 ? 1 : 0 ? 2 : 3;  // expect 3
    }
```

Precedence with logical OR
```C
    int main() {
        return 0 || 1 ? 10 : 20;  // expect 10
    }
```

Precedence with assignment
```C
    int main() {
        int x = 0;
        x = 1 ? 10 : 20;
        return x;         // expect 10
    }
```

Nested True branch
```C
    int main() {
        return 1 ? 0 ? 2 : 3 : 4; // expect 3
    }
```

Condition with Pointer
```C
    int main() {
        int x = 42;
        int *p = &x;
        return p ? *p : 0;     // expect 42
    }
```

## Invalid tests
Missing true expression
```C
    int main() {
        return 1 ? : 2;   // INVALID
    }
````

Missing Colon
```C
    int main() {
        return 1 ? 2;   // INVALID
    }
```

Missing false expression
```C
    int main() {
        return 1 ? 2 :;   // INVALID
    }
```

Missing condition expression
```C
    int main() {
        return ? 1 : 2;  // INVALID
    }  
```

