# ClaudeC99 Stage 46 command line args

## Task
  - Add support for command line arguments
  - Verify support of command line arguments 


## Test
### Integration Test
```C
    int puts(const char *);
     int main(int argc, char ** argv) {    // ARGS: Hello
         if (argc > 1) {
             puts(arg[1]);    // Expect Output: "Hello"
         }
         
         return argc;      // EXPECT Exit Code: 2        
     } 
```