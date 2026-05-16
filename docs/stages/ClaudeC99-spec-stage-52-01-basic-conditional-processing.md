# ClaudeC99 Stage 52-01 Basic Conditional Processing: #ifdef /#ifndef

## task
  Add basic conditional preprocessing support for check whether a macro name is defined
  
The stage should allow simple feature switches and include-guard-style patterns
Conditional state is based on whether a Macro is currently defined
Examples
```C
#ifdef NAME
...
#endif
```

```C
#ifndef NAME
...
#endif
```

```C
#ifdef NAME
...
#else
...
#endif
```

## Requirements

**Recognize conditional directives**
Add support for these directives:
```C
#ifdef
#ifndef
#else
#endif
```

**Macro-defined check**
Given:
```C
#define ENABLED
```

then 
```C
#ifdef ENABLED
```

is active

Given no definition for ENABLED THEN
```C
#ifndef ENABLED
```
is active

**

**Skip inactive regions**
inactive regions should not be emitted into the preprocessed output

Example
```C
#define ENABLED

#ifdef ENABLED
int main() {
    return 42;
}
#else
int main() {
    return 1;
}
#endif

```
should preprocess so that only the first `main` remains

**Do not expand macros inside skipped code**
This example should not fail because `BAD` is inside an active region
```C
#define ENABLED

#ifdef ENABLED
int main() {
    return 42;     //expeect exit code; 42
{
#else
BAD BAD BAD
```

allow object like macros inside conditional blocks
```C
#define ENABLED

#ifdef ENABLED
#define VALUE 42
#else
#define 1
#endif

int main() {
    return VALUE;    // expected exit code: 42
}
```

Allow simple include guard pattern
```C
ifndef HELPER_H_
#define HELPER_H_
int helper(void);
#endif
```

## out of scope
  - `#if`
  - `elif`
  - `defined(NAME)`
  - expression evaluation in conditionals
  - macro expansion inside conditional expressions
  - complex recovery from malformed nesting beyond clean errors
  - full include guard optimization
  - `#undef`

## Tests
`ifdef` true branch
```C
#define ENABLED

#ifdef ENABLED
int main() {
    return 42;     // expect status code 42
}
#else
int main() {
    return 1;
#endif
```

`ifdef` false branch
```C
#ifdef ENABLED
int main() {
    return 42;
}
#else
int main() {
    return 1;     // expect status code 1
}
```

`ifndef` true branch
```C
#ifndef ENABLED
int main() {
    return 42;   // expect status code 42
}
#else
int main() {
    return 1;
}
```

Macro defined inside active conditional
```C
#define ENABLED

#ifdef ENABLED
#define VALUE 42
#else
#define VALUE 1
#endif

int main() {
    return VALUE;    // expect status code 42
}
```

SKipped invalid source
```C
#define ENABLED

#ifdef ENABLED
int main() {
    return 42;    // expect status code 42
}
#else
THIS IS INVALID SOURCE THAT SHOULD SKIPPPED
#endif
   
```

## Invalid Tests
Missing `#endif`
```C
#define ENABLED

#ifdef ENABLED
int main() {
    return 42;
}
 // MISSING #endif   should error
```

`#else` without conditional

```C
#else         // INVALID
int main() {
    return 42;
}   
```

`#endif` without conditional
```C
   #endif       // INVALID   
   
   int main() {
       return 42;
   }
```

Duplicate `else`

```C
#define ENABLED

#ifdef ENABLED
int main() {
    return 42;
}
#else
int main() {
    return 1;
}
#else         // INVALID Duplicate #else
int main() {
    return 2;
}
#endif
```