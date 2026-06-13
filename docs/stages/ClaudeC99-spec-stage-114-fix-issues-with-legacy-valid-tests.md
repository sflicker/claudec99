# ClaudeC99 stage 114 fix issues with valid tests

## Task a number of tests have been imported into the project. about 24 are failing.
There is a failure report in docs/defects/legacy-project-test-failure-report.md

These will need to be fixed. From the summary in the report and steps to fix

1. Test file naming error (exit code out of range) | 3 |
    These return return negative numbers and the original project allowed that.
    This project is expecting the values to be unsigned 8-bit so a simple file
    rename to change the expected number from a negative to an unsigned 8-bit number.
    Also one test is expecting a value > 255. Adjust the expected in the file to fix it. 

2. Test file coding error (invalid C99) | 1 |
    The one test in this category seems to be using invalid number of initializers.
    modify the test .c to remove the last element in the initializer.

3.  Missing compiler feature | 2 |
    One failure has nested array initializer like '{{36, 1}, {2,3}} which is current unsupported. Add logic to support this. 

    Another failure is related to `subscript base must be an identifier`. However the
    report states this should be valic C99. Add logic to support this.

4. Compiler bug - local multidimensional array nested brace initialization. The report says these are failing due to a bug. Fix this

5. Compiler bug - FP array subscript write/read.
   fix this

6. Compiler bug - ternary operator with mixed FP/int branches
   fix this

## After fix
The tests from legacy project have not been added to git yet.
Once all the tests from legacy project are working. Move each
test file to one of the other existing directories appropriate for what it tests.
Once all the tests have been moved from the legacy_project folder remove it.

Commit all fixes and new test files originally from the legacy_project.

## Done criteria. All tests including the ones in valid/legacy_project should pass
