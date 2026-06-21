# ClaudeC99 stage 162 - add integration test to sysinclude tests for the zlib program from stage 158

## Task
Stage 158 fixed some issues compiling a program that uses the zlib libaries. However that test was not captured as an integration test.

In this task add a integration test to the integration_sysinclude that access the `zlib` includes and libraries that is similar to the program file in the stage 158 spec document. `docs/stages/ClaudeC99-spec-stage-158-compile-failure-with-external-library.md`

This test should also add .require logic so the test is only executed if the proper zlib headers/libraries are present.

Also add zlib as an optional library in the README.md file