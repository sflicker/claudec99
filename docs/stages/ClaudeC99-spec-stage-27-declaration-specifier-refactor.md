# ClaudeC99 Stage 27 Declaration specifier refactor

## Task
  - refactor declaration specifier parsing so declarations are built from a list
    of declaration specifiers rather than a fixed optional storage class plus one type specifier.

## Grammar Update
Current
```ebnf
    <declaration_specifiers>    ::= [ <storage_class_specifier> ] <type_specifier>
    <storage_class_specifier>   ::= "extern" | "static"
    <type_specifier> ::= <integer_type>
    <integer_type> ::= "char" | "short" | "int" | "long"
```

Updated
```ebnf
    <declaration_specifiers> ::= <declaration_specifier> { <declaration_specifier> }
    <declaration_specifier> ::= <storage_class_specifier>
                               | <type_specifier>
    <storage_class_specifier>   ::= "extern" | "static"
    <type_specifier> ::= <integer_type>
    <integer_type> ::= "char" | "short" | "int" | "long"
```

## Requirements
  - parse declaration specifiers as a list
  - detect duplicate storage classes
  - preserve current declarator behavior
  - keep rejecting storage classes at block scope
  - add clearer semantic validation for declaration specifiers

## Tests
- existing tests should continue to pass. If necessary some may be adjusted

Invalid cases
```C
    static extern int x;   // INVALID
```

```C
    extern static int x;   // INVALID
```

```C
    int int x;
```

```C
    static char int x;
```

```C
    static int x;
    
    int main() {
        static int x;     // INVALID
        return x;      
    }
```