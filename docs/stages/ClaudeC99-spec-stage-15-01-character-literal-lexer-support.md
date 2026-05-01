# ClaudeC99 Stage-15-01

## Task
  - add lexer support for character literal tokens

## Requirements
  - add lexer support for character literal tokens
  - supported initial forms:
```C
  'A'
  '0'
  '\n'
  '\t'
  '\r'
  '\\'
  '\''
  '\"'
  '\0'
   
  ```
 
  - The lexer should produce a single `TOKEN_CHAR_LITERAL` token containing:
    - The original spelling,
    - the evaluated integer value
    Examples
    - Source                           Value
       `A`                               65
       '\n'                              10
       '\t'                              9
       '\r'                              13
       '\\'                              92
       '\''                              39
       '\"'                              34
       '\0'                               0

  ## Out of Scope
    - These can be rejected for this stage
    ```C
       'ab'      // multi-character constant
       '\123'    // octal escape
       `\x41'    // hex escape
        L'A'     // wide character literal
        U'A'
        u'A'
    ```