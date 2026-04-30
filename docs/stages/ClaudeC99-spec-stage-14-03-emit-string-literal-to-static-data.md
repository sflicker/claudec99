# ClaudeC99 Stage 14-03

## Task
  - Generate static storage for string literals and emit the address of the string literal when used as an expression.

## Requirements
  - Assign each string literal expression a unique label
  - Emit string literal bytes into a static data section
  - include a null terminator after the literal contents
  - Generate the address of the string literal when the expression is evaluated
  - Existing tests must continue to pass

## Assembly 
  - String literals should be emitted into a read-only or static data section

## Recommended section
```asm
  .section .rodata
```

## Example
```C
   "hi"
```

emit:
```asm
   .section .rodata
   .Lstr0:
      .byte 104, 105, 0
```

## Expression codegen
When a string literal appears as an expression, generate the address of its label.
Example
```asm
    lea rax, [rel .Lstr0]
```

## Type Rule
A string literal has a logical type:
```C
   char[N]
```
where `N` is the string byte length plus one for the null terminator

In expression codegen, it may be treated as an address to the first byte of the static object

## Tests
- make a new class of tests stored in the directory test/print_asm.
- Each test should run the compiler with a test .c file to generate an .asm file
- Each test will have a corresponding .expected file to compare with the generated .asm file.
- Add the following tests along with .expected files for each
```C
    int main() {
        return "hi";
    }
```

```C
    int main() {
        return "";
    }
```

```C
    int main() {
        return "one";
    }
```

- test harness scripts should also be added. These should also be called when running the full test suite.

## Out of Scope
  - Assigning string literals to `char *`
  - Indexing string literals
  - `char *s = "hi"; return s[1]; `
  - Escape sequences
  - Char array initialization from string literals
  - Adjacent string literal concatenation
  - Libc calls such s `puts("hi")`

