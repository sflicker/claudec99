# ClaudeC99 stage 93 - bootstrap build workflow

## task
  - Update the project build system has three separation options for the compiler to use for ClaudeC99 builds
    - 1. "Normal Build" using GCC or Clang. Whichever is currently use.
    - 2. "Bootstrap Build" The current ClaudeC99 ccompiler that was built previously and passed the test suite.
    - 3. "Fallout back Build" this will use either the current ClaudeC99 compiler if available or fallback to GCC/Clang
  - Record which compiler was used to built compiler output for the build.
  - Add timeout guards for each test ran in the test suite or if the ClaudeC99 compiler is used to build itself.
    The goal of this is to prevent endless loops from tying up the development computer.

## Build system update
  - the build system should have some sort of configuration parameter so the above three configurations
    coule be used (Normal, Boostrap and Fallback)

## Version: Built by
  - Each build should should include a -D option with the compiler --version by using. suggested macro
    for this VERSION_BUILTBY
  - The version.c file should have something like
    ```C
    #ifndef VERSION_BUILTBY
    #define VERSION_BUILTBY  Default c compiler
    #endif
    ```
    
    and the version ouput should by updated to include a line to should the built by like
    ```C
            snprintf(version_buf, sizeof(version_buf),
                 "ClaudeC99 version %s.%s.%s.%05d\nBuildBy: %s",
                 VERSION_MAJOR, VERSION_MINOR, VERSION_STAGE,
                 VERSION_BUILD, VERSION_BUILTBY);
    ```                 
    
