# Stage 95-01 Kickoff

## Summary

Stage 95-01 is a documentation-only stage with no compiler code changes. The goal is to inventory every remaining fixed-capacity table and buffer in the compiler source, classifying each one with detailed metadata to enable future growable-storage implementation work.

This stage produces a comprehensive artifact (`docs/fixed-capacity-inventory.md`) that serves as a living reference document for planning the transition from fixed-size limits to dynamic allocation.

## Tokenizer Changes

None required.

## Parser Changes

None required.

## AST Changes

None required.

## Code Generation Changes

None required.

## Spec Issues Identified

Five typos in the specification (intent is clear):
1. "arbitrary likes" → "arbitrary limits"
2. "classfy" → "classify"
3. "savely" → "safely"
4. "artififact" → "artifact"
5. Filename typo: "capcity" → "capacity"

## Planned Artifacts

### docs/fixed-capacity-inventory.md
The primary deliverable: a comprehensive inventory of all fixed-capacity tables and buffers in the compiler. For each entry, the inventory records:
- Table/buffer name
- Current maximum capacity
- File/module location
- Overflow behavior (what happens when limit is exceeded)
- Pointer aliasing (whether entries are referenced by pointers elsewhere)
- Realloc safety (whether entries can safely move after reallocation)
- Priority ranking for self-hosting plan

This inventory serves as a living document that future stages can reference and check off items as they are converted to growable storage.

## Implementation Order

1. Audit all compiler source files for fixed-capacity limits
   - Search for array declarations with fixed sizes
   - Search for macro-defined constants used as limits
   - Search for overflow checks and error messages
   - Document each finding with complete metadata
2. Generate `docs/fixed-capacity-inventory.md` with all findings
3. Commit inventory document
4. Generate milestone summary artifact
5. Generate transcript artifact

## Decisions Made

- **No code changes** — This stage is purely documentation; no source modifications are made
- **No version.c update** — No code changes means no build version bump
- **No self-host test phase** — Nothing to compile or test since there are no code changes
- **No README update** — No user-visible capability changes
- **No grammar.md update** — No language feature changes
- **Living inventory document** — The inventory is placed in `docs/fixed-capacity-inventory.md` as a reference future stages will use to track conversion progress

## Ambiguities Resolved

None. Spec is clear: documentation inventory only, no code implementation.

## Files to Create or Modify

- docs/fixed-capacity-inventory.md (create - main deliverable)
- docs/kickoffs/stage-95-01-kickoff.md (create - this artifact)
- docs/milestones/stage-95-01-milestone.md (create - after completion)
- docs/sessions/stage-95-01-transcript.md (create - after completion)
