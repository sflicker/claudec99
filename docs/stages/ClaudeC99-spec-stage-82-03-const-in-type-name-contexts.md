# ClaudeC99 Stage 82-03 const in type-name contexts

## Feature
Allow const inside type names, not only variable declarations

Possible Example places
```C
sizeof(const char *)
sizeof(const int)
(int *)p
(const char *);
va_arg(ap, const char *)
```

## Grammar direction
Whenever the parser accepts a <type_name>, it should use thesame declaration-specifier / declarator machinery
as ordinary declarations, including:

type_specifier
type_qualifier
pointer declarator qualifiers

Grammar Update

```ebnf

<type_qualifier> ::= "const" | "volatile"

<specifier_qualifier_list> ::= <specifier_qualifier> { <specifier_qualifier> }

<specifier_qualifier> ::= <type_specifier> 
                         | <sign_specifier>
                         | <type_qualifier>

<parameter_declarator> ::= <specifier_qualifier_list> [ <declarator> ]

<struct_declaration> ::= <specifier_qualifier_list> <struct_declarator_list> ";"

<type_name> ::= <specifier_qualifier_list> [ <abstract_declarator> ]

<abstract_declarator> ::= <abstract_pointer_declarator>

<abstract_pointer_declarator> ::= "*" { <type_qualifier> } { "*" { <type_qualifier> }}

```

## Tests
```C
int main(void) {
    return sizeof(const char *) == sizeof(char *);   // expect 1
}
```

```C
int main(void) {
    return sizeof(const int) == sizeof(int);     // expect 1
}
```

```C
int main(void) {
    char *p;
    const char *q;

    p = "hello";
    q = (const char *)p;

    return q[0] == 'h';       // expect 1
}
