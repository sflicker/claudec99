# ClaudeC99 Stage 45 - Basic libc declarations and external calls

## Goals
  - Allow useful libc-style prototypes
  - support calls returning void *
  - support passing const char *
  - support external function declarations with definitions
  - ensure linker can resolve them through system libc
  - add simple compile/link/run tests using puts, strcmp, malloc/free





## Tests
### New Test Type and test harness
   Currently some of the *valid* tests have associated .expected files that are linked with libc and
   call `puts` and the output is checked against the expected. For this stage and going forward
   I want to put all the tests that call (and link against) standard library functions into a separate
   test/integration directory. This includes all the current *valid* tests that have a .expected file 
   and many more new files going forward. The run script in test/integration should use the following to
   build.

The test harness for test/integration will use additional files and no longer need the __[exitcode] in the file name
${BASENAME} - extracted from c_test_file.c
${BASENAME}.expected   - contains the expected output when running the compiled test program
${BASENAME}.libs     - contains additional -l type flags used for linking. example: "-lm" for linking again the math library
${BASENAME}.args     - command line args to pass the compile test program when executed
${BASENAME}.input    - input to be passed to the compiled test program through stdin redirection.
${BASENAME}.status   - if the exit code needs to be test it should be in this file otherwise the exit code will be assumed to be zero.

Test files can also use arbitrary extensions as long as the basename is the same if they need
a datafile to read as part of the test as long as these extensions don't conflict with the ones
defined above.

```shell

#.... build section

LFLAGS_FILE="${BASENAME}.lflags"

if [ -f "$LFAGS_FILE" ]; then
    EXTRA_LFLAGS=($(cat "$LFLAGS_FILE"))
else
    EXTRA_LFLAGS=()
fi

# Compile C -> ASM
echo "compiling: $SOURCE"
"$COMPILER" "$SOURCE"

# Assemble ASM -> object
echo "assembling: ${BASENAME}.asm"
nasm -f elf64 "${BASENAME}.asm" -o "${BASENAME}.o"

# Link object -> executable uses cc for linking
echo "linking: ${BASENAME}"
cc -no-pie "${BASENAME}.o" -o "${BASENAME}" "${EXTRA_LFLAGS[@]}"

## updated test logic to include the new companion files

```

The test harness should properly handle the .expected, .input, .args and .exticode files to
validate the test.

Existing tests in the test/valid that have the .expected companion files should be migrated/moved to this directory and new structure.
The __[exitcode] portion of the file names should be removed and a .exitcode file created with the
proper exit code inside the file.

## Integration Tests
place in the new test/integration folder

puts test
```C
    int puts(const char *);
    
    int main() {
        char * s = "42";
        
        puts(s);   // expected OUTPUT : "42"
        return 42; // expected EXITCODE: 42
    }
```

strcmp test
```C
    int strcmp(const char *a, const char *b);
    int puts(const char *);
    
    int main() {
        char * a = "ALPHA";
        char * b = "BETA";
        
        int result = strcmp(a, b);
        
        if (result !=0) {
            puts("DIFFERENT");
        else {
            puts("SAME");           // EXPECTED OUTPUT: "SAME"
        }
        return 0;
    }    
```

strlen test
```C
    typedef unsigned long size_t;
    size_t strlen(const char *s);
    int puts(const char *);
    
    int main() {
        char * s = "HELLO";
        size_t len = strlen(s);
        if (len == 5) {
            puts("CORRECT");       // EXPECTED OUTPUT: "CORRECT"
        } else {
            puts("INCORRECT);
        }
        return 0;
    }
```

malloc and free test
```C
    typedef unsigned long size_t;
    void * malloc(size_t size);
    void free(void *);
    
    int main () {
        int * block = malloc(256);
        free(block);
        return 0;
    }
```