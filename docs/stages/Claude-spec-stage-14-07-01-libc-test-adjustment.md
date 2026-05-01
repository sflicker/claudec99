# ClaudeC99 Stage 14-07-01

## Task
  - adjust Valid tests so output can optionally be compared with a known value.

## Description
   in stage 14-07 some Valid tests where added that generate output through the puts call.
   However no instructions where given how to check this output. This stage will add
   a way to check the output.

## Requirements
  - current valid test files are end with the extension .c. If there is a matching file with the same base name but with the extension .expected.
  - Compare the output from running the compiled test file with the values in the .expected file
  - If no .expected file is present don't do this comparison.
  - Back fill existing tests that are generating output and create .expected files.
  - update the test scripts in the test/valid directory to perform this extra optional check
