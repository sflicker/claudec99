# Claude Stage 47 add multi file integration test support

## Goals
  - Update Integration Test Harness to support multi files tests

Updated design of the integration test harness

1. Each Test will live in it's own subdirectory with the directory name the same as the base name of the main test file.
   test/integration/<basename>
2. The main function will be in the file <basename>.c
3. The test harness will compile then assemble each .c file in the directory to a separate object
4. Link all objects together with cc -no-pie
5. Current logic for additional support flies (recaping)
   - Append optional <basename>.libs contents to the link
   - Run test executable with optional <basename>.args and <basename>.input
   - Compare stdout with <basename>.expected if present
   - Compare exit statue with <basename>.status if present

In this stage all current tests in the test/integration folder will be migrated to the new format
with subdirectories.  

## Test
```C 
    ----- main .c test file  
    int add(int a, int b);
    
    int main() {
        return add(3,5);    // expect exit status: 8
    }
    ----- add.c 
    int add(int a, int b) {
        return a + b;
    }
    -----
```
