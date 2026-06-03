# ClaudeC99 stage 88 support hex and octal escapes

## Tasks
Support character hex and octal characters literals like
```C
'\x41'
`\101'
```

and hex and octal within strings
```C
"\x41"
"\101"
```

## Tests
character test
```c
#include <string.h>
int main() {
    char str[6];
    str[0] = 'H';
    str[1] = 'e';
    str[2] = '\x6c';    // l in hex
    str[3] = '\154';    // l in octal
    str[4] = 'o';
    str[5] = 0;

    return strcmp("Hello", str) == 0;  // expect 1
}
```

string test
```C
#include <string.h>
int main() {
    char * str = "He\x6c\154o";
    return strcmp("Hello", str) == 0;  // expect 1
}