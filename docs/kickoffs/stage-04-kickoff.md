I am building a C99 compiler in stages.

The stage 4 specification is defined in 
docs/stages/ClaudeC99-spec-stage-04-adding-unary-if-else-block.md

Read that file carefully.

Help me implement this stage step by step, follow the spec exactly. Do not introduce features beyond what is defined in the spec.

Preserve the current project structure, coding style, file organization, and naming conventions
unless the spec requires a change.

Focus on:
- tokenizer updates
- parser updates
- AST changes (if needed)
- code generation (X86_64 assembly in intel format)
- correctness with tests

## Instructions

1. Start by summarizing the required changes from the spec
2. Identify what must change from stage 3
3. Implement in this order:
    - tokenizer
    - parser
    - AST
    - code generation
4. Keep changes minimal, incremental, and consistent with the existing codebase.
5. Do not refactor unrelated code.
6. Do not add support for features not explicitly required by the stage 4 spec.

## At each Step
- explain what is changing and why
- show the updated code
- keep the changes minimal and incremental
- stop and wait for my confirmation before continuing to the next step.

Begin with the stage 4 summary and the tokenizer step only.