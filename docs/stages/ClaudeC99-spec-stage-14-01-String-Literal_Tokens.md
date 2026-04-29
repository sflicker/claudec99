# ClaudeC99 Stage 14.01

## Task
  - Add lexer support for string literal tokens

## Requirements
  - Add `TOKEN_STRING_LITERAL`
  - Recognize double-quoted string literals
  - Store the string contents without the surrounding quotes
  - Store both the literal byte array and the byte length
  - For this stage, only ordinary characters are supported
  - Reject newline byte before the closing quote
  - Reject end-of-file before the closing quote
  - Reject backslash escape sequences for now, with a clear diagnostic

## Examples
  ```C
      "Hello"
      "abc"
      ""
  ```
Token values:
```C
   "Hello" -> hello, length 5
   "abc"   -> abc,   length 3
   ""      -> "" ,   length 0 
```  

## Out of Scope
  - Parser support
  - AST support
  - Code generation
  - Escape sequences
  - Adjacent string literal concatenation

## Invalid Examples
```C
    "Unterminated
    "hello
    "line
    break"
    "A\n"
```

