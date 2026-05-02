# Generated Artifacts

This file defines the generated artifacts for the `implement-stage` skill.

- Use `STAGE_LABEL` in file names.
- 
- Use `STAGE_DISPLAY` in artifact titles.

## Kickoff Artifact

Generate this after reading the spec and before implementation

Template:

`${CLAUDE_SKILL_DIR}/templates/stage-kickoff-format.md`

Save to:

`docs/kickoffs/<STAGE_LABEL>-kickoff.md`

Purpose:

Capture the stage spec summary, derived stage values, planned changes, implementation order, ambiguities, and decisions made before implementation.

## Milestone Summary Artifact

Template:

`{CLAUDE_SKILL_DIR}/templates/milestone-summary-format.md`

Save to:

`docs/milestones/<STAGE_LABEL>-milestone.md`

Purpose:

Briefly summarize what the stage completed. Keep it concise and suitable for saving directly in the milestones directory.

No not produce a long implementation audit. Prefer a subsystem-oriented summary.

## Transcript Artifact

Generate this after implementation, testing and milestone summary are complete.

Template:

`${CLAUDE_SKILL_DIR}/templates/transcript-format.md`

Save to:

`docs/sessions/<STAGE_LABEL>-transcript.md`

Purpose:

Capture a fuller record of the session, including the plan, implementation steps, tests run, final result, generated artifacts, and notable decisions.

