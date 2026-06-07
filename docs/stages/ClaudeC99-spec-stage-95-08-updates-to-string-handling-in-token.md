# ClaudeC99 - stage 95-08 updates to string-literal handling in token

## Goal
Remove the fixed-size token string limit caused by:
```C
char value[MAX_NAME_LEN];
```

and replace it with pointer-plus-length token text storage while keeping `Token` fixed-size
and passable by value.

## Current Problem
Current token storage resembles:
```C
typedef struct Token {
    TokenType type;
    char value[MAX_NAME_LEN];
    int length;
    ...
} Token;

The lexer directly writes into the token
with something like
```C
token.value[i++] = decoded;
token.value[i] = '\0';
token.length = 1;
```

This causes several problems:
```text
string literals have an arbitrary maximum size
escaped string literals must fit in Token itself
Token is larger than necessary
passing Token by value copies the whole fixed buffer
future larger literals require increasing MAX_NAME_LEN
```

## New Design
Use fixed-size tokens that reference lexer-owned storage:
```C
typedef struct Token {
    TokenType type;

    const char *lexeme;
    size_t lexeme_len;

    const char *value;
    size_t value;
    ...
} Token;

```text
lexeme         raw source spelling, such as '\"abc\\n""
lexeme         raw source spelling length

value          cooked/decoded value, such as bytes: a b c newline
value_len      decoded byte length, excluding any added trailing '\0'
```

For identifiers and numbers, lexeme and value may sometimes point to the same text,
depending on how the lexer stores them

For String literals, they should usually differ
example
```C
"abc\n"
```
```text
lexeme = "\"abc\\n\""
value = "abc\n"

For operators, punctuator, ators and keywords lexeme and value can probably be left blank and
value deduced from the TOKEN_TYPE. A helper method could be added something like 
```C
const char * get_token_text(Token token) {
    switch(token.type) {
        case TOKEN_EQ:   return "==";
        ...
        default:
        return token.value;
    }
}
```

## Ownership Rule
The ownership rule should be explicit:
```
Lexer/string arena owns token text.
Token does not own text
Token copies are shallow and safe
Token is freed only when the owning lexer/string arena is destroyed.
```

So this is safe:
```C
Token a = lexer_next_token(lexer);
Token b = a;
```
Both tokens point to the same lexer-owner strings, and neither frees them.

## Lexer String-literal flow
The lexer should use `StrBuf` while decoding:
Something like
```
StrBuf decoded;
strbuf_init(&decoded);

while (!end_of_string) {
    if (current == '\\') {
        char ch = decode_escape(lexer);
        strbuf_append_char(&decoded, ch);
    }
    else {
        strbuf_append_char(&decoded, current);
        advance(lexer);
    }
}
```
Then copy the final decoded bytes into lexer-owned storage
token.value = lexer_store_bytes(lexer, decoded.data, decoded.len);
token.value_len =  decoded.len;

Then free only the temporary buffer:
```C
strbuf_free(&decoded);
```

## Important detai: Embedded NUL
keep value_len. Do not rely on strlen(token.value)

This must work correctly:
```
"abc\0def"

## Tests
tests using the following should be added if not present

```C
"very long string literal exceeding old MAX_NAME_LEN"
"abc\0def"
"line\nhex\x41octal\101"
"adjacent" "strings"
```

## Acceptance Criteria
```text
Token no longer contains char value[MAX_NAME_LEN]
Token uses pointer + length fields
strings literals are decoded through StrBuf
decoded strings are copied into lexer-owned storage
tokens can still be passed by value
embedded NUL string literals are handled using value_len
old string literal tests still pass
new long string literal test exceeds on MAX_NAME_LEN
c0->c1->c2 self-host validation passes
