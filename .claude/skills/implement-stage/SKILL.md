---

name: implement-stage
description: Implements a ClaudeC99 stage from a stage spec. Use when asked to read a stage spec, summarize required changes, then implement tokenizer, parser, AST, and code generation incrementally without adding extra features.
disable-model-invocation: true
------------------------------

Read this spec file first: $ARGUMENTS

Derive STAGE_LABEL from the spec filename using these rules:

1. Ignore the directory path and file extension.
2. Split the filename into hyphen-separated tokens.
3. Locate the token `stage`.
4. Starting immediately after `stage`, collect all consecutive tokens that match the regex `^[0-9]+$`.
5. Stop collecting when a token does not match this pattern.
6. Set `STAGE_LABEL` = `stage-` + the collected numeric tokens joined with `-`.
7. Set `STAGE_DISPLAY` = `STAGE_LABEL` with the first letter capitalized.

Examples:

* `ClaudeC99-spec-stage-01-minimal.md` → `stage-01`
* `ClaudeC99-spec-stage-07-02-for-statement.md` → `stage-07-02`
* `ClaudeC99-spec-stage-08-01-03-something.md` → `stage-08-01-03`

Validation:

* If no numeric tokens are found after `stage`, stop and report an error.
* Do not guess or infer missing stage values.

Usage:

* Use `STAGE_LABEL` for in file names of generated artifacts.
* Use `STAGE_DISPLAY` consistently inside all generated artifact names.
* Echo the derived value before implementation as:

   * `STAGE_DISPLAY: <value>`

Then do the following:

1. Read and summarize the stage spec.
2. Identify exactly what must change from the previous stage.
3. Call out any ambiguity, inconsistency, or grammar/spec issue before implementation.
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

* After reading the spec, produce a clearly labeled **Kickoff Summary** for saving and save into in the kickoffs directory.
* Before implementation, produce a clearly labeled **Planned Changes** section listing the expected file/module updates.
* After completing the stage, produce a clearly labeled **Milestone Summary** suitable for saving in the milestones directory.
  The Summary should brief be focusing on what was completed by the stage.
* After completing the stage, generate a transcript using `transcript-format.md` and save it in the sessions directory.
* Use `STAGE_LABEL` in all generated artifact filenames.

Grammar file in docs:
* docs/grammar.md should be kept up to date with the current grammar of the
   the compiler. If changes are made in the specification to the grammar
   they should be reflected in docs/grammar.md   

When relevant, use supporting files in this skill directory:

* `stage-kickoff-template.md` for the standard step-by-step structure
* `notes.md` for project-specific implementation reminders
* `transcript-format.md` for generating the session transcript
