# ClaudeC99 Stage 14.05

## Task
  - Add support for common escape sequences inside string literals

## Requirements
  - Update the lexer so string literals may contain supported backslash escape sequences

## Support Escapes
```C
  \n
  \t
  \r
  \\
  \"
  \0
```

The lexer should translate these source spellings into the actual byte values stored in the token

Examples
`"A\n" -> 65, 10`
`"A\"B" -> 65, 34, 66`
`"A\0B" -> 65, 0, 66`

The final null terminator is added by codegen and not counted as part of the literal token text length

Internal representation
String literals should continue to preserve:
```C
    bytes
    length
```
where length is the number of bytes in the literal after escape processing, excluding the implicit final null terminator.

```C
   "A\0B"
```
Has literal length 3.
emitted assembly is
```nasm
    db 65, 0, 66, 0
```

## Token printing
Update --print-tokens so escaped strings print in a stable testable form
  example   `TOKEN_STRING_LITERAL  A\n`

## AST
The AST string literal node should store the decoded byte sequence and length

The AST pretty-printer should also escape printable characters so expected output remains line-oriented
    example  StringLiteral:  "A\n"

## codegen
Codegen should emit decoded bytes using NASM `db`
Example  `"C"`  -> ```nasm  db 104, 105, 0```
          `"A\n"` -> ```asm db 65, 10, 0```
          `"A\0B"` -> ```asm db 65, 0, 66, 0```

## Runtime tests
Valid Tests:
```C
    int main() {
        char *s = "A\n";
        print s[1];     // expect 10
    }
```

```C
    int main() {
        char *s = "A\r";
        return s[1];     // expect 13
    }
```

```C
    int main() {
        char *s = "A\\B";
        return s[1];   // expect 92
    }
```

```C
    int main() {
        char *s = "A\"B";
        return s[1];   // expect 34
    }
```

```C
    int main() {
        char *s = "A\0B";
        return s[1];   // expect 0
    }
```

```C
    int main() {
        char *s = "A\0B";
        print s[2];    // expect 66
```

## Invalid tests
```C
    int main() {
        char *s = "\x41";   //ERROR invalid escape sequence
        return 0;
    }
```

## Out of scope
  - Hex escapes such as `\x41`
  - Octal escapes such as `\101`
  - Unicode escapes
  - Adjacent string literal concatenation
  - Character literals
  - Char array initialization from string literals

