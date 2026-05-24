# ClaudeC99 stage 64 long long types and LL literals support

## task
  - add support for the `long long` type
  - add support for `long long` literal prefix `LL`

Grammar update
note <integer_type> changing to <integer_type_specifier>
<sign_specifier> removed from <integer_type_specifier> as it appears in <declaration_specifier>

```ebnf

<declaration_specifiers> ::= <declaration_specifier> { <declaration_specifier> }

<declaration_specifier>  ::= <storage_class_specifier>
                            | <type_specifier>
                            | <sign_specifier>
                            | <type_qualifier>

<sign_specifier> ::= "signed" | "unsigned"

<type_specifier> ::= "void"
                     | <integer_type_specifier> 
                     | "_Bool"
                     | <typedef_name> 
                     | <enum_specifier> 
                     | <struct_specifier>
                     
<integer_type_specifier> ::= "char"
                           | "short"
                           | "int"
                           | "long"                     
                     
<integer_literal> ::= [0-9]+ <integer_suffix>

<integer_suffix> ::= <unsigned_suffix>
                    | <long_suffix>
                    | <unsigned_suffix> <long_suffix>
                    | <long_suffix> <unsigned_suffix>
                    
<unsigned_suffix> ::= "u" | "U"

<long_suffix> ::= "l" | "L" | "ll" | "LL"                                         
```
Parser/type-normalizer should be able to interpret combinations like
```C
signed char
unsigned char
short
short int
signed short
unsigned short int
int
signed
unsigned
long
long int
unsigned long
```

`long long` should be handled as two long specifiers. `long long` related combinations that should be supported now.
```C
long long
long long int
signed long long
singed long long int
unsigned long long
unsigned long long int
```

Literal updates to grammar should allow
```C
123       →  int, lomg, then long long as need
123U      → unsigned int, unsigned long, then unsigned long long   
123L      → long, then long long as needed
123UL     → unsigned long, then unsigned long long
123LU     → unsigned long, then unsigned long long
123LL     → long long
123ULL    → unsigned long long
123LLU    → unsigned long long
```

## New Types
add the following new types
```C
TYPE_LONG_LONG
TYPE_UNSIGNED_LONG_LONG
```  

## Properties for long long from x86-64 Linux LP64.
```C
   sizeof(long long)              == 8
   sizedof(unsigned long long)    == 8
   alighmemtn                     == 8
```

## Codegen
  - `long long` should be handled like `long`
  - load/store 8 bytes
  - arithmetic 64 bit register ops
  - comparison 64 bit comparisons
  - function args/returns: integer register types

## Predefined macro
  Add this predefined macro to the macro table from code
  ` #define __SIZEOF_LONG_LONG__ 8`
  
## Updates to limits.h
  Add these define to the limits.h file in <project-root>/test/include
  ```C
  #define LLONG_MIN (-9223372036854775807LL - 1LL)
  #define LLONG_MAX 9223372036854775807LL
  #define ULLONG_MAX 18446744073709551615ULL
  ```

## tests

```C
int main(void) {
    return sizeof(long long);   // expect 8
}
```

```C
int main() {
    return sizeof(unsigned long long);  //expect 8
}
```

```C
long long f(void) {
    long long x;
    x = 42LL;
    return x;
}

int main(void) {
    return f();           // expect 42
}
```

```C
int main(void) {
    unsigned long long x;
    x = 42ULL;
    return x == 42ULL;      // expect 1
```

```C
int main(void) {
    long long x;
    x = -5ULL;
    return x < 0;         // expect 1
```

```C
typedef long long i64;
typedef unsigned long long u64;

int main(void) {
    i64 a;
    u64 b;
    a = 40LL;
    b = 2UUl;
    return a + b;      // expect 42
```

TEsts for limits.h updates
```C
#include <limits.h>
int main() {
#ifdef defined(CLAUDEC99_LIMITS_H) && defined(LLONG_MIN)
    return 0;      // expect 0
#else
    return 1;
#endif
}
```

```C
#include <limits.h>
int main() {
#ifdef defined(CLAUDEC99_LIMITS_H) && defined(LLONG_MAX)
    return 0;      // expect 0
#else
    return 1;
#endif
}
```

```C
#include <limits.h>
int main() {
#ifdef defined(CLAUDEC99_LIMITS_H) && defined(ULLONG_MAX)
    return 0;      // expect 0
#else
    return 1;
#endif
}
```

### Invalid tests
```C
int main() {
   long long long x;         // invalid
   return 0;
}
```

```C
int main() {
    short long x;     // invalid
    return 0;
}
```

```C
int main() {
    unsigned signed long long x;     // invalid
    return 0;
}

```

```C
int main() {
    long long char x;     // invalid
    return 0;
}

```