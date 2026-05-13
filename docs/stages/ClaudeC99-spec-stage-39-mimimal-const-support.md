# ClaudeC99 Stage 39 - minimal const support

## Goals
  - parse `const`
  - store `const` on the base/pointee type
  - accept `const char *`, `const void *`
  - reject assignment to simple const objects
  - leave full pointer-level const enforcement later

## Task
Support parsing these
```C
    const int x;
    const char *s;
    const void *p;
    int strcmp(const char *a, const char *b);
```

## Token
  add keyword `const`

## Grammar Update
```ebnf

    <type_qualifier> ::= "const"
    
    <declaration_specifier>  ::= <storage_class_specifier>
                            | <type_specifier>
                            | <type_qualifier>
    
```

Enforce direct modification reject
```C
    const int x = 3;
    x = 4;   // reject
```

and reject this
```C
    const char* s;
    *s = 'x';
```

Enforcing this is out of scope
```C
    const int x = 3;
    int * p = &x;
    *p = 10;     // semantic/type-checking enforcement is out of scope for this stage.
```

## Semantic Checking
Stage 39 parses and records `const` qualifiers and enforces assignment rejection
   for direct const-qualified lvalues, such as assigning to a `const int` variable.

Full pointer const-correctness is out of scope. In particular, this stage does not
need to reject conversions that discard const qualifiers, such as 
assigning `const int *` to `int *`, nor does it need to reject writes through pointers-to-const
such as `*p = value` when `p` has type `const int *`.

## Tests
Add Tests

## Codegen
Store const const in a similar manner as other other of of similar types
storage of const objects in read only segments is out of scope for this stage. 