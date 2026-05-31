# ClaudeC99 stage 82-04 minimal `volatile` handling

## task
add minimal handling of `volatile`
    - volatile parses as a type qualifier
    - volatile is preserved in the type
    - volatile has no special codegen behavior yet

## Token update
add a keyword token for `volatile`

## Test
```C
struct S {
    volatile int flag;
};

int main(void) {
    struct S s;
    s.flag = 42;
    return s.flag;    // expect 42
}
```
