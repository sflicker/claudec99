# ClaudeC99 Stage 54 undef support

## Task
  - Add support for undefining object-like and function-like macros

## In-Scope
  - Parse `#undef NAME`
  - remote `NAME` from the macro table if present
  - allow `#undef` of an undefined macro as a no-op
  - Make `#ifdef` / `#ifndef` reflect the new state
  - Make later macro expansions reflect the new state

## Out-of-scope
  - `#if defined(NAME)`
  - expression evaluation in `#if`
  - standard headers
  - command-line `-D` / `-U`

## Test
Basic Test
```C
#define X 1
#undef X

#ifndef X
int main() {
    return 42;      // expect status code: 42
}
#else
int main() {
    return 1;
}
#endif
```

Make sure macro is used before it's undef
```C
#define VALUE 42
int main() {
    int x = VALUE;
#undef VALUE
#ifdef VALUE
    x += 10;
#endif
    return x;     // expected return value: 42
}
```