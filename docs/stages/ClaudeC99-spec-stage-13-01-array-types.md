# ClaudeC99 Stage 13.1

## Task
  - Update the compiler to to add array declarations
  - Update the Type list to include an Array type
  - Update codegen to handle local variable array storage (stack) allocation

## Token Updates
  - `[` - left bracket
  - `]` - right bracket

## Grammar updates
  ```ebnf 
      <declaration> ::= <type> <identifier> [ "[" <integer_literal>  "]" ] [ "=" <expression> ] ";"
  ```
  
## Requirements
### Array Type
  - Add a new type kind: ARRAY
  - track:
    - element type
    - array length
    - total size
    
### Declaration Support
    - support arrays of current types
    - - Examples
      - `int a[3];`
      - `char a[10];`
      - `short s[4];`
      - `long values[4];`
      - `int *ptrs[2];`  pointers of other types as well

### Constraints
      - Array size must be:
        - integer literal
        - must be greater than zero
        - Reject
          - `int a[0]`
          - `int a[-1]`
          - `int a[x]`
## Codegen
    - Allocate stage space:
        - `sizeof(element_type) * length`
    - Store base address in symbol table (same as scalars, but larger size)
## Semantics 
    - Array variables represent storage, not pointer variables
    - Arrays are NOT assignable
    ```C
        int a[3];   
        int b[3];   
        a = b;    // ERROR
    ```

## Out of Scope
    - Array indexing (`a[i]`)
    - Pointer Arithmetic
    - Array-to-pointer decay
    - Array initializers
    - Multi dimensional arrays
    - Global Arrays

## Tests
### Basic allocation
```C 
    int main() {
        int a[3];
        return 0;
    }
```

### Mixed locals
```C 
    int main() {
        int x;
        int a[3];
        int y;
        return 0;
    }
    
```

### Different types
```C
    int main() {
        char a[4];
        long b[2];
        return 0;
    }
```