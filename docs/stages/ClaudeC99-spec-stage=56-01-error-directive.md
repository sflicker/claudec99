# ClaudeC99 Stage 56-01 `#error` directive

## Task
  Add support for the `#error` preprocessing directive
  
## In-Scope
  - Recognize `#error message`
  - Emit a clean preprocessor error containing the message text
  - Only trigger `#error` in active conditional regions
  - Ignore `#error` inside inactive conditional regions

## Out of Scope
  - #warning
  - fancy diagnostic formatting
  - source-map perfection

## Tests
```C

#if 0
#error this should be skipped
#endif

int main() {
 if 
   return 42;    // expect 42
}
```

## Invalid Test
`#error` should stop complication. so this should be this test should be in the invalid category
```C
#error unsupported platform

int main() {
    return 0;
}
```