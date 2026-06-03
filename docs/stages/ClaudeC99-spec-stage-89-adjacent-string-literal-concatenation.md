# ClaudeC99 Stage 89 - adjacent string-literal concatenation

## goal
support standard c adjacent string literals
```C
"hello " "world"
```
as if they were one string literal
```C
"hello world"
```

## Grammar update
```ebnf
<string_literal> ::= TOKEN_STRING_LITERAL { TOKEN_STRING_LITERAL }
```

## Tests
```C
int main() {
    char *s;
    s = "hello " "world";
    return s[6] == 'w';   // expect 1
}
```

```C
int main() {
    return strlen("abc" "def") == 6;    //expect 1
}
```

```C
int main() {
    char *s;
    s = "\x41" "B";
    return s[0] + s[1] - 89;    // expect 42
}
```

```C
#include <string.h>
int main(void) {
    char *s;
    s = "error: "
        "expected token";
    
    return strcmp(s, "error: expected token") == 0;   // expect 1
}

```C
char *msg = "hello " "world";

int main() {
    return msg[6] == 'w';    // expect 1
}

## Invalid Tests

```C
char * a = "hello" + "world";   // INVALID
```

