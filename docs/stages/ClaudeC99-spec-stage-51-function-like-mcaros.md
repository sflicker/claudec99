# ClaudeC99 Stage 51 - function like macros

## Task
  Add support for simple function like macros
Example
```C
#define ADD(a,b)  ((a) + (b))

int main() {
    return ADD(20,22)
}
```

## Requirements
  - parse function-like macro definitions
    ```C
    #define NAME(arg1, arg2) replacement 
    ```
  - require no whitespace between macro name and `(` in the definition
   ```C
    #define F(X) x      // function like macro
    #define F (x)       // object like macro replacement starts with (x) 
  ```
  - expand macro invocations in ordinary source
  - substitute arguments into the replacement list
    - Macro arguments may be arbitrary preprocessing-token sequences, not just literals.
    - Argument collection should split on commas only at parenthesis depth zero.
  - Support nested parentheses in marco arguments
    ```C
      ADD(1, ADD(2,3)) 
   ```
  - Support zero-argument function like macros:
   ```C
    #define VALUE() 42
   ```
  - argument count should match exactly
  - Keep object-like macro support from stage 50 working

## Out of scope
  - stringification `#`
  - token pasting `##`
  - variadic macros
  - conditional processing
  - full recursive macro expansion beyond what is needed for normal object/function-like expansion
  - macro hygiene edge cases

## Tests
Basic cases
```C
#define ADD(a,b)  ((a) + (b))

int main() {
    return ADD(20,22);       // expect status code: 42
}
```

```C
#define SQUARE(x)  ((x) * (X))

int main() {
    return SQUARE(8);    // epect status code: 64
}
```

```C
#define ANSWER() 42

int main() {
    return ANSWER();    // expect status code 42
}
```

```C
#define ADD(a,b) ((a) + (b))

int main() {
    return ADD(1+2,3);    // expect status code 6
}
```

Nested macros
```C
#define ADD(a,b) ((a) + (b))

int main() {
    return ADD(ADD(1,2),3);   // expect status code 6
```

## Invalid Case
Argument count does not match
```C
#define ADD(a,b) ((a) + (b))

int main() {
    return ADD(1,2,3,4);    // INVALID ARGUMENT COUNT DOE SNOT MATCH..
```