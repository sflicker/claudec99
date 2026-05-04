# ClaudeC99 Stage-19   comma operator

## Task
  - Add for support for the comma operator for expressions only
  - Do Not add comma-separated declarations in this stage
```C
    expr1, expr2   
```
The comma operator evaluates the left expression first, discards its value
then evaluates the right expression. The result of the whole comma expression
is the value and type of the right expression
Example
```C
    int main() {
        int x = 0;
        return (x = 1, x + 2);   // expect 3
    }
```

## Semantics
The comma operator:
  - has the lowest precedence among expression operators
  - is lower precedence than assignment
  - is left-associative
  - evaluates operands left to right
  - produces the right operand's value and type
  - is not an lvalue at this stage
Examples:
```C
  a, b, c   -> parses as (a, b), c
  
  x = 1, y = 2  -> parses as (x = 1), (y = 2) 
  ```
To assign the result of a comma expression, paratheses are required:
```C
    x = 1, 2);  // x becomes 2
```

### Important parser distinction
** Do Not Confuse the comma operator with comma separators
Commas used in function arguments and parameter lists
are separators, not comma operators

```C
    foo(a, b);        // two arguments 
    int f(int a, int b);  // with comma separators between each
```

This is a comma operator used as one argument
```C
    foo((a,b));   // one argument
```

## Grammar updates
```ebnf
    <expression> ::= <assignment_expression> { ", " <assignment_expression> }
```
## Parser and AST
  - Add an AST Node type for comma expressions
  - comma operator should be treated as a binary operator
  - Parse comma expressions at the <expression> level
  - Preserve comma separators in function calls and parameter lists

## Type Handling
  - result type of a comma expression is the right operand's type

## Codegen
  - evaluate left operand
  - discard result
  - evaluate right operand
  - leave result in `rax`/`eax`
For chained comma expressions
```C  
  a, b, c, d
```
The generated behavior should evaluate:
```C
   a
   b
   c
   d
```
In order, with the final result from `d`.

## Tests
### Suggested Valid Tests to Add
Basic result value
```C
    int main() {
        return (1, 2);   // expect 2
    }
```
Left to right evaluation
```C
    int main() {
        int x = 0;
        return (x = 1, x + 2);   //expect 3
```

Multiple comma chain:
```C
    int main() {
        int x = 0;
        return (x = 1, x = x + 2, x = x + 3, x);   // expect 6
    }
```

Comma has lower precedence than assignment:
```C
    int main() {
        int x = 0;
        int y = 0;
        x = 1, y = 2;
        return x + y;   // expect 3
    }
```

Assignment to comma result requires parentheses around comma expression
```C
    int main() {
        int x = 0;
        x = (1, 2);
        return x;     // expect 2
    }
```

Comma inside return expression
```C
    int main() {
        int x = 0;
        return x = 4, x + 1;   //expect 5
    }
```

Comma in `if` condition
```C
    int main() {
        int x = 0;
        if (x = 1, x) {
            return 10;    // expect 10
        }
        return 20;
    }
```

Comma in `while` condition
```C
    int main() {
        int x = 0;
        int y = 0;
        while(x = x + 1, x < 4) {
            y = y + x;
        }
        
        return y;    // expect 6
    }
```

Comma as one function argument with parentheses
```C
    int f(int x) {
        return x;
    }
    
    int main() {
        int a = 1;
        return f((a = 3, a + 4));  // expect 7
    }
```

### Suggested Invalid tests to add
Missing right operand
```C
    int main() {
        return (1,);   // INVALID
    }
```

Missing left operand
```C
    int main() {
        return (,1);   // INVALID
    }
```

Bad function argument comma
```C
    int f(int x) {
        return x;
    }
    
    int main() {
        return f(1,);
    }
```

## Out of Scope
  - multiple declarators in one declaration, such as `int a, b;`
  - comma separated variable initialization declarations
  - comma operator as lvalue
  - full initializer-list syntax
  - array initializer lists
  - `for` declaration lists
  - variadic functions
  - preprocessor behavior
