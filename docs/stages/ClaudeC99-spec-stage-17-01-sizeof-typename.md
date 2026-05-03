# ClaudeC99 Stage-17-01 sizeof Type Names

## Task
  - Add support for `sizeof(<type>)`
  - This stage will be limited to `sizeof` applied to explicit type name inside parenthesis
  - Examples
    - `sizeof(int)`
    - `sizeof(char)`
    - `sizeof(int)`
    - `sizeof(long)`
    - `sizeof(int *)`
    - `sizeof(char *)`

## Tokens
  - `sizeof`  ->  `TOKEN_SIZEOF`
  - The tokenizer should recognize sizeof as a keyword not an identifier

## Grammar Update
```ebnf
<unary_expression> ::= "sizeof" "(" <type> ")"
                    | <unary_operator> <unary_expression>
                    | <postfix_expression>

```

## Requirements
  - `sizeof(<type>)` produces the size in bytes of the given type.
  - Supported sizes for this stage
    - `char`    1
    - `short`   2
    - `int`     4
    - `long`    8
    - `pointer` 8
  - Pointer types should return 8 bytes on the current x86-64 target
  - The result should be emitted as an integer constant.
  - For this stage, the result type should be `long`

## Parser and AST
  - Add an AST node for `sizeof` type-name expressions, for example:
    - `AST_SIZEOF_TYPE`
  - Ths node should store the parsed type.
  - The parser should distinguish this form from ordinary parenthesized expressions by recognizing
    - `sizeof ( <type> )`
  - For this stage, if `sizeof` is not followed by "(" <type> ")", the parser should report an error

## Codegen
  - Generate the type size as a constant value
  - Examples
  ```C
      sizeof(char)    -> mov rax, 1
      sizeof(short)   -> mov rax, 2
      sizeof(int)     -> mov rax, 4
      sizeof(long)    -> mov rax, 8
      sizeof(int *)   -> mov rax, 8 
  ```

## Out of Scope
  - sizeof expressions
    - examples of unsupported in this stage
      - `sizeof x`
      - `sizeof (x)`
      - `sizeof (a + b)`
  - size_t
 

## Tests
```C
    int main() {
        return sizeof(char);  // expect 1
    }
```

```C
    int main() {
        return sizeof(short);  // expect 2
    }
```

```C
    int main() {
        return sizeof(int);   // expect 4
    }
```

````C
    int main() {
        return sizeof(long);  // expect 8
    }
````

````C
    int main() {
        return sizeof(int *);  // expect 8
    }
````