# ClaudeC99 stage 56-05 standard predefined macros

## Tasks
  - add a small set of predefined macros


```C
#define __STDC__ 1
#define __STDC_VERSION__ 199901
#define __CLAUDEC99__ 1
```

## Requirements
  - Built-in macros are defined and inserted into the macro table before preprocessing starts
  - These macros are available in `#if`, `#ifdef` and normal source macro expansion
  - They behave like ordinary object-like macros
  - User -D definitions should be rejected if incompatible redefinitions
  - `#undef` should work on predefined macros

## Out of scope
  - __DATE__
  - __TIME__
  - __GNUC__
  - __GNUC_MINOR__
  - architecture/platform macros
  - exact GCC/CLANG compatibility

## Tests
```C
#if ___STDC__ == 1 && __STDC_VERSION__ >= 199901
int main() {
   return 42; // expect 42
}
#else
int main() {
    return 1;
}
```