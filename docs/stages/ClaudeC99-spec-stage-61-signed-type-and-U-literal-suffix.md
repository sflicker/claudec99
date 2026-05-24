# ClaudeC99 stage 61 signed type and U suffix support.md

## Tasks
  - Add signed support for the signed keyword and type
  - Add support for the U suffix for integer type literals

## Tokens
add a keyword token for `signed`

## Grammar Updates

```ebnf

<declaration_specifiers> ::= <declaration_specifier> { <declaration_specifier> }

<declaration_specifier>  ::= <storage_class_specifier>
                            | <type_specifier>
                            | <sign_specifier>
                            | <type_qualifier>

<sign_specifier> ::= "signed" | "unsigned"

<integer_type> ::=  "char"
                  | "short"
                  | "int"
                  | "long"

<integer_literal> ::= [0-9]+ <integer_suffix>

<integer_suffix> ::= [Uu] 
                   | [Ll] 
                   | [Uu][Ll] 
                   | [Ll][Uu]

```

** Literal type rules for this stage **
  - 123 → int or long if does not fit in int
  - 123U → unsigned int or unsigned long if it does not fit unsigned int
  - 123L → long
  - 123UL → unsigned long
  - 123LU → unsigned long

** Allowed combinations and normalization rules for this stage**
```C
int                         → int
signed                      → int
unsigned                    → unsigned int
signed int                  → int
unsigned int                → unsigned int

char                        → char
signed char                 → signed char
unsigned char               → unsigned char

short                       → short
signed short                → short
unsigned short              → unsigned short
short int                   → short
signed short int            → short
unsigned short int          → unsigned short

long                        → long
signed long                 → long
unsigned long               → unsigned long
long int                    → long
signed long int             → long
unsigned long int           → unsigned long
```

Reject:
```C
signed unsigned int x;
unsigned signed int x;
signed void x;
unsigned void x;
signed struct S x;
unsigned enum E x;
long long x;       // reject for this stage
```

## Tests
```C
int main(void) {
    signed char d;
    c = -1;
    return c < 0;     // expect 1
}
```

```C
int main(void) {
    unsigned int x;
    x = 42U;
    return x == 42U;   //expect 1
```

```C
int main(void) {
    unsigned long x;
    x = 42UL;
    return x == 42UL;    // expect 1
}
```

```C
typedef signed char int8_t;

int main(void) {
    int8_t x;
    x = -5;
    return x < 0;   // return 1
}
```

## Invalid Tests
```C
 int main() {
    signed unsigned int x;    // INVALID
    return 0;
 }
```

```C
 int main() {
    unsigned signed int x;    // INVALID
    return 0;
 }
```

```C
 int main() {
    signed void x;    // INVALID
    return 0;
 }
```

```C

struct X {
  int x;
};

 int main() {
    unsigned struct X x;    // INVALID
    return 0;
 }
```
