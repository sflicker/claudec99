# ClaudeC99 Stage 91-01 Support struct-by-Value parameters and returns

## Tasks
Support forms like
```C
typedef struct Token {
    char *value;
    int length;
} Token;

static Token finalize(Token token) {       // struct Token passed by value as a parameter
    token.length = (int)strlen(token.value);
    return token;                          // struct Token returned by value
}
```

Semantically, this function receives a copy of the caller's `Token`, modifies the copy, and 
returns the modified modified copy.

So this call:
```C
Token t;
t = finalize(t);
```
Means
```text
copy caller t into finalize's local parameter token
modify token.length
copy token back as return value
assign returned Token into t
```

## Tests
```C
#include <stdio.h>

typedef struct Token {
    char *value;
    int length;
} Token;

static Token finalize(Token token) {
    token.length = (int)strlen(token.value);
    return token;
}

int main(void) {
    Token t;

    t.value = "hello";
    t.length = 0;

    t = finalize(t);

    return t.length;    // expect 5
}
```

Prove-by-value parameter behavior
```C
#include <stdio.h>

typedef struct Token {
    char *value;
    int length;
} Token;

static Token finalize(Token token) {
    token.length = (int)strlen(token.value);
    return token;
}

int main(void) {
    Token t;
    Token u;

    t.value = "hello";
    t.length = 99;

    u = finalize(t);

    return t.length == 99 && u.length == 5;    // expect 1
}
```