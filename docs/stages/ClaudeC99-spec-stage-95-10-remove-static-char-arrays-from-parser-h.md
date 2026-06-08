# ClaudeC99 Stage 95-10 convert static char arrays in parser.h to const char *

## Task
parser.h has numerous struct that contain an char array with a static size of MAX_NAME_LEN

Example
```C
typedef struct {
    char tag[MAX_NAME_LEN];
    int  is_defined;
} EnumTag;
```
in this example convert `char tag[MAX_NAME_LEN]` to use `const char * tag`. 
replace copy operations like memcpy and strncpy to assignments and other relevant code.
Example
```C
    StructTag new_st;
    new_st.type = NULL;
    strncpy(new_st.tag, tag, sizeof(new_st.tag) - 1);
    new_st.tag[sizeof(new_st.tag) - 1] = '\0';
```

Should be replace with
```C
    StructTag new_st;
    new_st.type = NULL;
    new_st.tag = tag;
```

Since there are numbers structs with this form in parser.h make a checklist with an item for each
conversion. After each conversion run the test suite. after it successfully passes commit
the code changes for that conversions, update the checklist then repeat for the next item.

After all the conversions update the fixed-capacity-inventory.md

## Tests
add tests where appropriate to ensure test coverage
