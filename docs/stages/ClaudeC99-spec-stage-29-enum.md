# ClaudeC99 enum support

## Task
  - Add c-style enum support as integer constants
Example
```C
    enum Color {
        RED,
        GREEN,
        BLUE
    };
    
    int main() {
        return GREEN;   //expect 1
    }
    
```

## Token update
add keyword `enum`

## Grammar update
```ebnf
<type_specifier> ::= <integer_type> | <typedef_name> | <enum_specifier>
<enum_specifier> ::= "enum" <identifier> "{" <enumerator_list> "}"
                    | "enum" "{" <enumerator_list> "}"
                    | "enum" <identifier>
                    
<enumerator_list> ::= <enumerator> { "," <enumerator> } [", "]

<enumerator> ::= <identifier> [ "=" <constant_expression> ]                    
```

## Support the following examples
1. **Anonymous `enum` constants**
```C
    enum {
        A,
        B,
        C
    };
    
    int main() {
        return C;   // expect 2
    } 
```

2. **Named `enum` tag**
```C
    enum Color {
        RED,
        GREEN,
        BLUE
    };
    
    int main() {
        enum Color c = BLUE;
        return c;    // expect 2
    } 
```

3. **Explicit values**
```C
     enum Error {
         OK = 0,
         BAD = 10,
         WORSE
     };
     
     int main() {
         return WORSE;    // expect 11
     }
```
### Note currently only literals are allowed for explicit values

4. **Character constants as values**
```C
    enum Letters {
        A = 'A',
        B = 'B'
    };
    
    int main() {
        return B;    // expect 66
    } 
```

5. **Trailing Comma**
```C
    enum Color {
        RED,
        GREEN,
        BLUE,
    };
    
    int main() {
        return BLUE;     // expect 2
    } 
```

## Suggested Invalid Tests
**Duplicate enumerator in same scope**
```C
    enum {
        A,
        A     //INVALID
    };
    
    int main() {
        return 0;
    }
```

**Missing `enum` tag**
```C
    int main() {
        enum Missing x;     // INVALID
        return 0;
```

**Invalid non literal explicit value (Out of scope)**
```C
    enum {
        A = 1 + 2            // INVALID 
    };
    
    int main() {
        return A;
    }
```

## Out of Scope
- full constant expressions in enumerator values
- forward enum declarations without definition
- fixed underlying enum types
- enum compatibility rules beyond simple same-tag handling
- typedef enum
