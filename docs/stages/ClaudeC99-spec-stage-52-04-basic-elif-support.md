# ClaudeC99 Stage 52-04 basic `#elif` support

## task
  Add support for `#elif`
  For this stage `elif`should support the same limited expression form 
  as stage 52-03
  
```C
#elif 0
#elif 1
#elif 42
```

## In-Scope
  - #elif <integer_literal>
  - multiple #elif branches
  - #elif after #if
  - #elif after #ifdef
  - #elif after #ifndef
  - nested use with the existing conditional stack
  - correct branch selection
    - first true branch wins
    - later branches are skipped once one branch has been taken

## Out of scope
  - full #elif expressions
  - defined(NAME)
  - macro-expanded `#elif` expressions
  - `#elifdef`/ `#elifndef`

## Tests
```C
#define ENABLED
#ifndef ENABLED
int main() {
  return 1;
}
#elif 1
int main() {
    return 42;   // expect status code 42
}
#endif
```

```C
#define ENABLED
#ifndef ENABLED
int main() {
  return 1;
}
#elif 114
int main() {
    return 42;   // expect status code 42
}
#endif
```


```C
#define ENABLED
#ifdef ENABLED
int main() {
  return 1;    // expect status code 1
}
#elif 1
int main() {
    return 42;  
}
#endif
```
```C
#define ENABLED
#ifndef ENABLED
int main() {
return 1;    
}
#elif 0
int main() {
return 42;  
}
#elif 1
int main() [
    return 2;   // expect status code 2
#endif
```
