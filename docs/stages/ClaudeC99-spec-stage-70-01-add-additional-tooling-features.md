# ClaudeC99 Stage 70.01 - Add additional tooling features

## Task
  - Adding a versioning scheme for the compiler. 
  - This new code modules version.h and version.c files.
  - The actual version number will be hardcoded into version.c
  - Each stage should increment the build number and store it in version.c
  - If there is a way to hook into cmake such that each time it is
  - ran the version number is updated. If necessary the version number can
  - be kept in a separate text file if that makes updating an automatic process.
  - If it's not possible to hook into cmake to update a build number then
  - alternative approaches could be considered to automatically update the version number.
  - An second approach if no better approach is found would be to wrap the build
  - process into a custom script that both calls the appropriate cmake/make option
  - to build along with having logic to update the build number.

  - The format of the version number will be: 00.00.00000000.00000 
    - the first two digits are the major version. for now set to 00.
    - the second two are the minor version. For now set to 01.
    - The third number is a stage number. This stage is 70.01. This would
    - translate into a number for the version of 00700100. The first four
    - digits are for main stage number. in This case 70. The next two digits are for this
    - sub stage number in this case 01. The last two digits are for the sub sub stage
    - number. In this case 00.

  -  The application should display the version number with the --version flag.
    - The version.c method should provide a message that generates a version
    - string that also includes the name of the application "ClaudeC99" and 
    - should contain an extra information field (hard coded information) that will initially
    - be blank but should if it was populated.
  
  - A Second tooling feature to be added is a new flag --max-error=[number]. Where number
  - is the max number of errors before the compiler quits. If 0 then the compiler will
  - continue through the whole file without stopping. Other numbers cause the compiler to 
  - continue until [number] of errors has been encountered. 
  - The cc99 script should also be updated so can accept and pass parameters onto the ccompiler app.