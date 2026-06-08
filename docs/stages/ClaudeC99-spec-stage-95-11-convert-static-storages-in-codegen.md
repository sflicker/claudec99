# ClaudeC99 stage 95-11 - convert static storages in codegen

## Task 
convert static storages in codegen.h and codegen.c

codegen.h has several structs that include a form like

```C
typedef struct {
    char name[MAX_NAME_LEN];
    ...
} LocalVar;
```

Convert this to `const char * name` and update the related code to use assignments instead of mem/str copies in codegen.c

There is also the multidimensional array ``char user_labels[MAX_USER_LABELS][MAX_NAME_LEN];

This should be convertable to a `vec` that holds `const char *`

Make a list of conversions and process them one by one. Running the test suite after each.

After all stages are completed update the fixed-capacity-inventory.md

## Tests
add tests where appropriate to ensure test coverage