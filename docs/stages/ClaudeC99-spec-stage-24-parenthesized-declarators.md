# ClaudeC99 Stage 24 Parenthesized declarator

## Task
  - Add support for parenthesized declarators as grouping syntax
    in the general declarator grammar
  - examples
    ```C
        int (*p);
        int (**pp);
        int (*p) = &x;
    ```
  - For this stage, parenthesized should be treated as **declarator grouping**.
  - This should allow existing pointer/object/function declaration behavior to be
  - expressed with c-style parentheses
  - Function pointers are out of scope for this stage.
  - 

## Updated Grammar
```ebnf

    <direct_declarator> ::= <identifier>
                       | <identifier> "(" <declarator> ")"
                       | <identifier> "[" [ <integer_literal> ] "]"
                       | <identifier> "(" [ <parameter_list> ] ")"
```

## Requirements
  - parse parenthesized declarators
  - preserve existing behavior for ordinary declarations
   Examples
   ```C
      int x;
      int *p
      int a[10];
      int f(int x); 
  ```
   - Support grouped pointer declarators
   ```C
       int (*p);
       int (**pp); 
   ```
   - Support grouped pointer declarations at file scope and block scope
   - support grouped pointer parameters
   ```C
       int read(int (*p)) {
           return *p;
       } 
   ```
   - Support grouped pointer return declarations only if they resolve to
     already supported return types.
    ```C
       int (*f()) {
          return 0;
       }
   - ```
     This may be deferred if necessary or create too much ambiguity

## Stage Semantics
  - This stage should only accept parenthesized declarators that resolve
    to already supported types.
Allow:
```C
   int (*p);
   int (**p);
   char (*s);
   long (**lp);
   int (*a[10]);
  ```

Out of Scope
```C
    int (*fp)(int);    // function pointer
    int (*f())(int);   // function returning pointer to function
    int (p*)[10];      // pointer to array types
```

## Valid Tests
Local grouped pointer declarator
```C
    int main() {
        int x = 7;
        int (*p) = &x;
        return *p;     // expect 7
    }
```

Double pointer with grouping
```C
    int main() {
        int x = 9;
        int *p = &x;
        int (**pp) = &p;
        return **p;    // expect 9
    }
```

File-scope grouped pointer
```C
    int x = 5;
    int (*p);
    
    int main() {
        p = &x;
        return *p    // expect 5
    }
```

Static grouped pointer
```C
    static int x = 11;
    static int (*p);
    
    int main() {
        p = &x;
        return *p;    // expect 11
    }
```

Parameter grouped pointer
```C
    int read(int (*p)) {
        return *p;
    }
    
    int main() {
        int x = 13;
        return read(&x);    // expect 13
    }
```

Extern grouped pointer declaration
```C
    extern int (*p);
    int x = 17;
    int (*p) = &x;
    
    int main() {
        return *p;    // expect 17
    }
```

## Invalid Tests
Function Pointer not support in this stage
```C
    int (*fp)(int);    //INVALID
    
    int main() {
      return 0;
    }
```

Pointer to array unsupported
```C
    int (*p)[10];    // INVALID
    
    int main() {
        return 0;
    }
```

Function pointer parameter unsupported
```C
    int call(int (*fp)(int)) {   // invalid
        return fp(3);
    }
    
    int main() {
        return 0;
    }
```

function returning pointer to function unsupported
```C
    int (*fp())(int);    // invalid
    
    int main() {
        return 0;
    }
```