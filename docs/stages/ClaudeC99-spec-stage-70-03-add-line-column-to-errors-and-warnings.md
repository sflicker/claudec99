# ClaudeC99 stage 70-03 add line,col to error and warning messages

## Task
  - using the line and column information stored in the token instances add <file>:<line>:<col> information
    to error and warning message. This information should appear at the beginning of each error message. 

Here is a current error message:
`error: cannot declare variable 'x' of type void`

Update it some it would be something like

`test_void_var_decl__cannot_declare.c:2:10: error: cannot declare variable 'x' of type void`

Make a similar change for warning messages as well.

## Tests
  - If this causes `invalid` tests to fail update the test harness so it uses a `contains` like
  - partial string comparison instead of a full string comparison. Where the proper error message
  - is part of the error output and ignore the rest like line and column positions.
