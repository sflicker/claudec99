I am building a C99 compiler in stages.

The stage 3 specification is defined in docs/stages/ClaudeC99-spec-stage-03-equality-expressions.md

Read that file carefully.

Help me implement this stage step by step, follow the spec exactly. Do not introduce features beyond what is defined in the spec.

Focus on:
- tokenizer updates
- parser updates
- AST changes (if needed)
- code generation (X86_64 assembly in intel format)
- correctness with tests

## Instructions

1. Start by summarizing the required changes from the spec
2. Identify what must change from stage 2
3. Implement in this order:
   - tokenizer
   - parser
   - AST
   - code generation

## At each Step
- explain what is changing and why
- show the updated code
- keep the changes minimal and incremental

Pause at each step and wait for confirmation before continuing.
