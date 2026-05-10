# ClaudeC99 Stage 30 Struct definition, layout and sizeof

## Task
  - introduce struct definitions and support sizeof

## Tokens
add `struct`

## Grammar Update
```ebnf
    <type_specifier> ::= <integer_type> 
                       | <typedef_name> 
                       | <enum_specifier>
                       | <struct_specifier>
                       
    <struct_specifier> ::= "struct" <identifier> "{" <struct_declaration_list "}"
                          | "struct" <identifier>
                          
    
    <struct_declaration_list> ::= <struct_declaration> { <struct-declaration> }
    
    <struct_declaration> ::= <type_specifier> <struct-declarator-list> ";"
    
    <struct_declarator_list> ::= <declarator> { "," <declarator> }
```

### Supported in this stage
   - only named struct definitions and references
   - struct declarations support fields with ordinary declarators

### Out of Scope
  - struct member access
  - anonymous structs
  - struct field initializers
  - struct storage-class specifiers
  - struct member access with "." and "->"
  - struct assignment, parameters, return values and initializers
  - bit-fields

## Tests

Base support
```C
    struct Point {
        int x;
        int y;
    };
    
    int main() {
        return sizeof(struct Point);     // expect 8
    }
```

```C
    struct Mixed {
        char c;
        int i;
    }
    
    int main() {
        return sizeof(struct Mixed);   // the expected could vary depending on the padding used in the implementation.
    }
```
Correct expected in above test depending on if padding is used or not for the `char` field.

```C
    struct Point {
        int x;
        int y;
    };
    
    int main() {
        struct Point p;
        return sizeof(p);     // expect 8
```
