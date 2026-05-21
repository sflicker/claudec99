# ClaudeC99 Stage 57-02 token pasting with `##`

## Task
Add support for token pasting in function-like and object-like macros

Example
```C
#define JOIN(a, b) a && b

int main(void) {
    int foobar;
    foobar = 42;
    return JOIN(foo, bar);   // expect 42
}
```

## In-Scope
  - `##` in macro replacement lists
  - pasting macro arguments with ordinary tokens
  - pasting two macro arguments
  - pasting identifiers
  - pasting identifier + number where useful
  - clean errors for invalid pasted tokens

## Out of Scope
  - Variadic macros
  - __VA_ARGS__
  - ## comma swallowing extensions
  - Full GCC compatibility edge cases

## Tests - use the above example as a test for this feature