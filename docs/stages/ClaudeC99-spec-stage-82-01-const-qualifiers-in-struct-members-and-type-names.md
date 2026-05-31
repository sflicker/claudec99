# ClaudeC99 Stage 82-01 `const` Qualifiers in Struct Members and Type Names

## Goal
  - Fix the remaining self-hosting gap:
     const qualifier in struct members / type-names

  - This project already supports `const` in ordinary declarations and parameters, with const-assignment checks
    and pointer const-discard diagnostics. This stage should extend that support into the two currently failing
    forms from the self-host build:
    ```C
    struct S {
        const char *name;
    };
    ```
    and type-name contexts such as:
    ```C
    sizeof(const char *);
    ```
    or casts/builtin type operands if those appear in compiler modules.

## Task
   - This substage will specifically handle `const` in struct/union member declarations.
   - Related `const` updates will occur in later substages.

## Feature
allow type qualifiers in aggregate member declarations:
```C
struct Field {
    const char *name;
    const int kind;
};

union Value {
    const char *text;
    const int code;
}
```
This should use the same qualifier machinery already used for ordinary declarations.

**Important distinction**

These two mean different things:
```C
const char *name;              // pointer to const char
char * const name;             // const pointer to mutable char
```

**For struct members, both should preserve the qualifier in the member type.**

## Tests
```C
struct Field {
  const char *name;
};

int main(void) {
  struct Field f;
  f.name = "hello";
  return f.name[0] == 'h';     // expect 1
}
```

```C
struct Field {
   const int kind;
};

int main(void) {
   struct Field f = {42};
   return f.kind;   // expect 42
}
```

## invalid tests
```C
struct Field {
  const int kind;
};

int main(void) {
  struct Field f = {1};
  f.kind = 2;    // INVALID
  return f.kind;
}
```