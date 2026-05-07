# ClaudeC99 Stage 24 Parenthesized declarator

## Task
  - Add support for parenthesized declarators
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
```