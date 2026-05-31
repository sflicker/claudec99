# ClaudeC99 Stage 79 General Lvalue compound assignment

## Goal
  - Support these forms
```C
p->cap *= 2;
p->cap += 1;
arr[i] += 2;
tokens[i].kind += 1;
*p += 3;   
```

Instead of parsing compound assignment as 'Identifier plus operator', parse it as ordinary assignment syntax

Currently this code snippet
```C
typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} GrowBuf;

...

static void gbuf_push(GrowBuf *g, char c) {
    if (g->len + 1 >= g->cap) {
        g->cap *= 2;     // THIS LINE IS PRODUCING THE ERROR: expected ';', got '*=' ('*=')  
        g->data = realloc(g->data, g->cap);
        if (!g->data) { fprintf(stderr, "error: out of memory\n"); exit(1); }
    }
    g->data[g->len++] = c;
}
```
Appears to be incorrectly handling the form `g->cap *= 2;`
With the error: `expected ';', got '*=' ('*=')`

The parser code needs to handle 
`p->cap *= 2;`  conceptually like `p->cap = p->cap * 2;`

All compound operators needs to work correctly for this form
```C
+=
-=
*=
/=
%=
<<=
>>=
&=
^=
|=
```

## Tests
```C
struct Buf {
    int cap;
};

int main(void) {
    struct Buf b;
    struct Buf *p;
    
    p = &b;
    p->cap = 21;
    p->cap *= 2;
    
    return p->cap;         // expect 42
}
```

Array/Member combination
```C
struct Token {
    int kind;
};

int main(void) {
    struct Token tokens[2];
    
    tokens[1].kind = 40;
    tokens[i].kind += 2;
    
    return tokens[1].kind;      // expect 42
```

Pointer dereference:
```C
int main(void) {
   int x;
   int *p;
   
   x = 21;
   p = &x;
   *p *= 2;
   
   return x;     // expect 42
}
```