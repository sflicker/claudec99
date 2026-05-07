# ClaudeC99 defect 1 - don't should token numerical codes in error message.

I compiled the following program

```C
int main() {
    int a[10];
    int (*p)[10] = &a;
    return 0;
}
```

Which should not compile as of stage 23.

When I compiled I got the following error:

error: expected token type 20, got 24 ('(')

Instead of seeing token numerical codes i would like to see some sort of human readable token names.
