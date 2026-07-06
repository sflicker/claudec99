# ClaudeC99 stage 176 - semicolon after a while gives error

## task
The following seems to be a compile error

```C
int main() {
   while(0);
   return 0;
}
```

```
cc99 while_error.c 
/tmp/while_error.c:7:12: error: expected expression, got ';'
```

This error does not occur with gcc.

I'm also getting a similar error with a for(;;) like

```C
int main() {
    for(;;);
    return 0;
}
```

```
cc99 for_error.c 
/tmp/for_error.c:7:12: error: expected expression, got ';'
```
