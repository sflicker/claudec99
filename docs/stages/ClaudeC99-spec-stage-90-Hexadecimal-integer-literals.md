# ClaudeC99 Stage 90 - hexadecimal integer liters

## Task
Add support for hexidecimal integer literals
Examples
```C
0x0
0x1
0x2A
0X2A
0xffffffff
0x10U
0x10UL
0x10ULL
```

hexidecial integer literals can also include suffixes
```C
U
L
UL 
LU
LL
ULL
LLu
```

## Tests
```C
int main() {
    return 0x2A;    // expect 42
}

```C
int main() {
    unsigned long x;
    x = 0X2AUL;
    return x == 42UL;    // expect 1
}

## Invalid Tests
```C
int main() {
    return 0x;
}
```

```C
int main() {
    return 0xG1;
}
```