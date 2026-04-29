# ClaudeC99 Stage 14-00

## Task
  - Add a command line option to the compiler for a lexer only mode
  - Add a new testing directory "test/print_tokens"
  - Add lexer tests coverage for the current token values supported

## Requirements 
### Print tokens flag and output
    - the flag should be --print-tokens
    - In this mode only the lexer is executed and processes the entire file
    - Output is something like
    - [###] TOKEN:: <value>            TOKEN_TYPE: <type>
    - where the left most column contains the token number starting with 1. The number digits displayed should be
      be enough for the token. Ex. use one digit if 1-9, two if 10-99, three, if 100-999 etc.
    - The <value> is the text value of the token. Only the first 15 character to be display. If the token is
      longer truncate at 15 and add an ...
    - The <type> is the token type.

### Create Lexer test harness
  - in the new test/print_tokens directory create a testing structure similar to the one used in print_ast
  - one or more scripts should be added to run the tests.
  - For each test create two files.
    - test files should begin with test_
    - then continue with the name of the test. example "test_function_call" 
    - a file with a .c extension. which could be either a complete c program or just a fragment
    - also create a .expected file with the expected results.
  - The test harness should loop through add the files
    - For each set of files
      - First the ccompiler should be executed with the --print-tokens flag for the .c files
      - Next the output should be compared with the .expected file
      - A pass or fail should be printed
  - The after all the tests the total number of pass or fail should be display
  - This set of tests should be executed when the other project tests and contribute to the total number of tests.

### Create tests for the existing token types
  - Tests should be generated for all the current token types. One test for each type
  - Tests should also be generated based on a representative set of small .c program to cover the
      current feature of the project.  Ideally something in the neighborhood of 20-40 of this type at this point.
  - 