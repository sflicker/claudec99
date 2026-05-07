# ClaudeC99 Stage 23 storage class basics

## Goal
  - add basic support for file-scope storage-class specifiers
```C
  extern
  static 
  ```
  - This stage should distinguish
```C
    int x;           // file scope object definition, external linkage
    extern int x;    // dclaration only, no storage emitted
    static int x;    // file scope object definition, internal linkage
  ```
  - and similarly for functions
```C
    int f();        // external-linkage function declaration
    extern int f(); // external-linkage function declaration
    static int f(); // internal-linkage function declaration 
  ```

## Grammar Updates
Current
```ebnf
    <function_definition> ::= <type_specifier> <declarator> <block_statement>
    <declaration> ::= <type_specifier> <init_declarator_list> ";"
```

Update to
```ebnf
    <function_definition> ::= <declaration_specifiers> <declarator> <block_statement>
    <declaration> ::= <declaration_specifiers> <init_declarator_list> ";"
    <declaration_spedifiers> ::= [ <storage_class_specifier> ] <type_specifier>
    <storage_class_specifier> ::= "extern" | "static"
```

This stage will only support storage class before the type. 

## Tokenizer
Add tokens for new keywords
`extern`
`static`

## Rules for `extern`
File-scope object declarations
Support:
```C
    extern int x;
    extern char c;
    extern short s;
    extern long l;
    extern int *p
```
An `extern` file-scope object without declaration without an initializer declares
the object but does not allocate storage and does not emit `.bss` or `.data`

so
```C
     extern int s;
```
Should not emit
```asm
    x: resd 1
```
But this should be valid
```C
    extern int x;
    int x = 7;
    
    int main() {
        return x;    // expect 7
    }
```
or
```C
    extern int x;
    
    int main() {
        return x;    // expect 7
    }
    
    int x = 7;
```
`x` could also be defined in a separate translation unit but this stage will get examples and
tests to a single file.

Function declarations
Support
```C
    extern int puts(char *s);
    extern int add(int a, int b);
```
For functions, `extern` mostly means the same thing as ordinary prototype for this stage.
```asm
    extern int f();
    
    int f() {
        return 3;
    }
    
    int main() {
        return f();
    }
```

Invalid `extern` cases for this stage
Do not allow `extern` declarations with initializers
```C
    extern int x = 3;    //INVALID
```
Do not allow the same identifier to switch storage linkage
```C
    extern int x;
    static in x;
```

## Rules for `static`
Support
```C
    static int x;
    static int y = 3;
```
Rules:
  - a `static` file-scope object has internal linkage
  - It still has static storage duration
  - Uninitialized `static` objects are zero-initialized
  - Initialized `static` objects go in `.data`

Example
```C
    static int counter; 
    
    int inc() {
        counter = counter + 1;
        return countsr;
    }
    
    int main() {
        inc();
        incd();
        return counter;   //expect 2
    }
```
 Initialized static global
```C
    static int x = 0;
    
    int main() {
        return x;
    }
```