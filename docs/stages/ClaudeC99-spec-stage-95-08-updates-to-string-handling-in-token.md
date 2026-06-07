# ClaudeC99 - stage 95-08 updates to string-literal handling in token

## Task
Change handling of string-literal handling in token
Currently Token is structured like
```C
typedef struct {
    TokenType type;
    char value[MAX_NAME_LEN];
    int length;
    ...
} Token;
```
This forces a fixed string size with an arbitrary limit because Token must be passed by value.

Also the lexer is directly building the token value with code like
```C
...
token.value[i++] = decoded;
...
```

Also the token instance owns the string memory.

## Updates
  1. make the lexer maintain owner ship of all string token values
  2. Change the Token struct to contain a `const char *` that points to the lexer owned string value instead 
of a char[MAX_NAME_LEN]
  3. change the lexer to use StrBuf to dynamically build string literals. When finished it will create a 
     a copy of the string in it's store. The token will have it's pointer assigned to the value in the 
     lexer's string store. The token will also have separate pointer to the decoded version of the string.

So the updated Token struct will be something like
```C
typedef struct Token {
    TokenType type;
    const char * lexeme;
    size_t lexeme_len;
    
    const char * value;
    size_t value_len;
    ...
}

Token parsing in the lexer would be changed to something using a StrBuf. 
