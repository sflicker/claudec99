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
  - `${CLAUDE_SKILL_DIR}/reference/generated-artifacts.md`
  - `${CLAUDE_SKILL_DIR}/notes.md`

Also always read the `README.md` file in the project root

## Analysis and Planning Phase

Then do the following:

1. Read and summarize the stage spec.
2. Identify exactly what must change from the previous stage.
3. Call out any ambiguity, inconsistency, or grammar/spec issue before implementation.
4. Produce the *Kickoff artifact*. Delegate this artifact generation to 'haiku-stage-artifact-writer'
4. Propose an implementation plan in this order:

   * tokenizer
   * parser
   * AST (if needed)
   * code generation
   * tests
   * documentation updates (if needed)
   * commit the changes (if needed) including a commit message.

## Implementation Phase:

* Follow the spec exactly.
* Do not introduce features beyond what is defined in the spec.
* Keep changes minimal and incremental.
* Prefer existing project style and naming conventions.
* Explain what is changing and why before showing code.
* Pause after each major step and wait for confirmation before continuing.
* If the spec is ambiguous, inconsistent, or contains grammar errors, stop and point them out before implementing.
* Add tests listed in the spec if a similar test is not already present.
* Additional tests may be added to fill test coverage gaps.
* Update the src/version.c code module to contain the current stage. See the section on version.c below for the formatting rules.
* During the feature implementation phase use the normal compiler (GCC or clang).
* After the stage's features changes have been implemented and all the tests (old and new) are passing then commit the code.

## Self Host Test Phase

The self-host phase runs the full bootstrap cycle using a single command:

```
./build.sh --mode=self-host
```

This command is self-contained — it always starts with a fresh GCC cmake
build to guarantee C0 is GCC-built, regardless of what `build/ccompiler`
currently contains:

1. Archives any existing `build/ccompiler-c0/c1/c2` to `build/previous/`
2. Runs `cmake` build to produce a fresh GCC-built `build/ccompiler-c0`
3. Runs the full test suite with C0
4. Bootstraps C0 → C1 (`build/ccompiler`); runs full test suite with C1
5. Makes a checkpoint git commit (so C2's build number exceeds C1's)
6. Bootstraps C1 → C2 (`build/ccompiler`); runs full test suite with C2
7. Reports final versions of C0, C1, and C2

`build/ccompiler` is left as C2 at the end of the run.

If any issues arise during the bootstrap, stop and prompt for guidance to
either fix the issue(s) or end the self-hosting test.

After the `--mode=self-host` run completes:
* Update `docs/self-compilation-report.md` with the stage number, date,
  method, any issues found and fixed, and the C0/C1/C2 result table.
* Commit the updated report.

Then proceed with the output generation steps below.

## Documentation Phase
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
   they should be reflected in docs/grammar.md. Delegate this task to `haiku-stage-artifact-writer`

README.md
* Update `README.md` only where the completed stage changes the user-visible compiler capability, project status, or test totals.
* Assume coding agents will always read the `README.me` as the first step towards understanding the current stage of the project.
* Delegate this task to `haiku-stage-artifact-writer`
* Preserve the existing README structure and writing style.
* Usually update:
   * the “Through stage ...” line,
   * the relevant bullet under “What the compiler currently supports,”
   * the “Not yet supported” section if the stage removes a limitation,
   * the aggregate test totals and suite breakdown.
* Do not rewrite unrelated sections.
* Do not add implementation-level detail that belongs in the milestone or transcript.
* If the stage does not change user-visible capabilities, say that no README update is needed.

version.c
* update the stage number in `version.c`.
    * The stage number is a 8 digit number
    * The first four digits are the stage number.
    * The next two are the substage number.
    * The next two are the sub-substage number.
    * Given a stage number like 70.03.0
    * The stage number would be 00700300
    * the `#define VERSION_STAGE  "00700100"` object macro in the version.c code module should be 
      updated during the stage implementation following the above rules.
* Update the BuildBy 
    * Uses the compiler used for the build and record it's `--version` as <BUILDBY>
    * when compiling version.c use a `-D VERSION_BUILDBY= <BUILDBY> from the previous step.

## DO NOT do update-supplemental-docs 
Do not perform the update-supplemental-docs skill  