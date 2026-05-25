# ClaudeC99 stage 70-02 line-col-fields-in-token

## Task
- Add `line`, `col` token positions (1 based counters) fields that are set to the beginning of each token by the lexer
- Add `File*` field for each token.
- Update the --print-tokens compiler option so line,col are in the output.

When the lexer processes a file it will keep track of the current File*, line and column. 
When a token is created by the lexer it will set the current values on the new token.
The line and column should point to the first character of the token.
When an include file is read by the preprocessor the current file/line/col should be saved
and the File* should be set to the include file and line,col should reset to (1,1).
After reading the include file the saved positions should be restored. This likely will require
changes to both the preprocessor and lexer.

For the --print-tokens output a new column should be added after the number column. This will 
make the position column second. The column should be format something like "[<line>:<col>]"
where <line> is the line number and <col> the column. The width of both should be set during 
execution to use the smallest number digits. Example. if the max column for all the lines was 221 then the 
column portion of the output will width of 3 digits. Both line and col should have leading zeros
in the output. Example. If the max line is 1304 and max col is 221 then for line 20, column 1 should be
formatted like [0022:001].

## Out-of-scope
  - using tokens in error messages

## Test
Update the existing print_tokens tests to include positions data for each line. 