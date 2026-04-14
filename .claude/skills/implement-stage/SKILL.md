---
name: implement-stage
description: Implements a ClaudeC99 stage from a stage spec. Use when asked to read a stage spec, summarize required changes, then implement tokenizer, parser, AST, and codegen incrementally without adding extra features.
disable-model-invocation: true
---

Read this spec file first: $ARGUMENTS

then:
1. Summarize the required changes from the spec.
2. Identify what must change from the previous stage.
3. Implement in this order:
    - tokenizer
    - parser
    - AST (if needed)
    - code generation
    - tests

Rules:
- Follow the spec exactly.
- Do not introduce features beyond what is defined in the spec.
- Keep changes minimal and incremental.
- Explain what is changing and why before showing code.
- Pause after each major step and wait for confirmation before continuing.
- If the spec is ambiguous, inconsistent, or contains grammar errors, stop and point them out before implementing.
- Prefer existing project style and naming conventions.

When relevant, use supporting files in this skill directory:
- `stage-kickoff-template.md` for the standard step-by-step structure
- `notes.md` for project-specific implementation reminders