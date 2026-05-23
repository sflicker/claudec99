# ClaudeC99 stage 60-01 static predefined target macros

## Task
Add the following predefined macros into the macro table before preprocessing begins.

```C
#define __x86_64__ 1
#define __linux__ 1
#define __unix__ 1
#define __LP64__ 1
#define _LP64 1

#define __CHAR_BIT__ 8
#define __SIZEOF_CHAR__ 1
#define __SIZEOF_SHORT__ 2
#define __SIZEOF_INT__ 4
#define __SIZEOF_LONG__ 8
#define __SIZEOF_POINTER__ 8
#define __SIZEOF_SIZE_T__ 8

```

## Implementation
These should be injected directly from the preprocessor code, not a separate .h file

## Tests
Add tests to validate these macros

