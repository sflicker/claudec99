---

name: implement-stage
description: Implements a ClaudeC99 stage from a stage spec. Use when asked to read a stage spec, summarize required changes, then implement tokenizer, parser, AST, and code generation incrementally without adding extra features.
disable-model-invocation: true
allowed-tools: Bash(python3 *)
------------------------------

Read this spec file first: $ARGUMENTS

Derived stage values:

!`python3 "${CLAUDE_SKILL_DIR}/scripts/stage-label.py" "$ARGUMENTS"`

Use the derived values above as authoritative.

Validation:

* If the stage-label script reports an error, stop and report the error.
* Do not guess or infer missing stage values.

Usage:

* Use `STAGE_LABEL` for in file names of generated artifacts.
* Use `STAGE_DISPLAY` consistently inside all generated artifact names.
* Echo the derived value before implementation as:

   * `STAGE_DISPLAY: <value>`

Before beginning, read these supporting files when relevant:
  - `${CLAUDE_SKILL_DIR}/reference-artifacts.md`
  - `${CLAUDE_SKILL_DIR}/notes.md`

Then do the following:

1. Read and summarize the stage spec.
2. Identify exactly what must change from the previous stage.
3. Call out any ambiguity, inconsistency, or grammar/spec issue before implementation.
4. Produce the Kickoff artifact.
4. Propose an implementation plan in this order:

   * tokenizer
   * parser
   * AST (if needed)
   * code generation
   * tests
   * documentation updates (if needed)
   * commit the changes (if needed) including a commit message.

Implementation Rules:

* Follow the spec exactly.
* Do not introduce features beyond what is defined in the spec.
* Keep changes minimal and incremental.
* Prefer existing project style and naming conventions.
* Explain what is changing and why before showing code.
* Pause after each major step and wait for confirmation before continuing.
* If the spec is ambiguous, inconsistent, or contains grammar errors, stop and point them out before implementing.

Output Requirements:

* Before implementation, produce a clearly labeled **Planned Changes** section listing the expected file/module updates.
* After completing the implementation and tests, delegate generation of the Milestone Summary artifact and Transcript Artifact to 'haiku-stage-artifact-writer;'
* The artifact write must use `STAGE_LABEL' in filenames and `STAGE_DISPLAY` in artifact titles
* Use `STAGE_LABEL` in all generated artifact filenames.

Multi-model routing:
* Use the current/main model for implementation
* use `haiku-stage-artifact-writer` for final generated artifacts
* Do Not delegate compiler source changes to Haiku
* Do Not use Opus unless the spec is ambiguous, architectural, or repeated implementation/debugging attempts

When delegating to `haiku-stage-artifact-writer`, pass:
  - spec path
  - STAGE_LABEL
  - STAGE_DISPLAY
  - summary of completed changes
  - tests run and final test result
  - files changed
  - artifact types to generate

Grammar file in docs:
* docs/grammar.md should be kept up to date with the current grammar of the
   the compiler. If changes are made in the specification to the grammar
   they should be reflected in docs/grammar.md   

README.md - update the project README.md based on the existing format in that file.
