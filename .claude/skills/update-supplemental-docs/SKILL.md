---
name: update-supplemental-docs
description: Refreshes the project's supplemental status documents after one or more stages land — the feature checklist, the full parse-tree, and the project-features / project-status snapshots. Reads the latest existing document of each kind as a template, then writes updated copies reflecting the current HEAD.
---

## Goal

Bring the four supplemental documents up to date with the current state of the
project (HEAD), driven by the stage milestones / specs that have landed since
the last refresh. The four documents are:

1. **Feature checklist** — `docs/outlines/checklist.md` (updated in place)
2. **Full grammar parse tree** — `docs/other/stage-<NN>-parse-tree.md` (new file)
3. **Project status snapshot** — `status/project-status-through-stage-<NN>.md` (new file)
4. **Project features snapshot** — `status/project-features-through-stage-<NN>.md` (new file)

`<NN>` is the latest completed stage label (e.g. `91`, or a substage like
`82-05` if that is the most recent). The checklist is the one document that is
**edited in place**; the other three are **new files** that follow the most
recent existing file of their kind as a template.

## Step 1 — Determine the stage range

Find the latest stage already reflected in each document, and the latest stage
actually completed:

```bash
# latest stage captured by each snapshot family
ls status/project-status-through-stage-*.md | sort
ls status/project-features-through-stage-*.md | sort
ls docs/other/stage-*-parse-tree.md | sort

# latest completed stage (specs and milestones)
ls docs/stages/   | grep -oE 'stage-[0-9]+(-[0-9]+)*' | sort -V | tail -5
ls docs/milestones/ | grep -oE 'stage-[0-9]+(-[0-9]+)*' | sort -V | tail -5

# the last stage already in the checklist
grep -oE '^## Stage [0-9]+' docs/outlines/checklist.md | tail -3
```

The set of stages to fold in is everything from the document's last-captured
stage up to the latest completed stage. If everything is already current, say so
and stop.

## Step 2 — Read what changed

For every new stage in range, read its milestone (concise) and, when more detail
is needed, its spec:

```bash
cat docs/milestones/stage-<NN>-milestone.md
cat docs/stages/ClaudeC99-spec-stage-<NN>-*.md   # only if the milestone is thin
```

Milestones are the primary source — each one summarises Tokenizer / Parser /
AST / Codegen / Tests / Docs changes and the final passing test count. Capture,
per stage: the user-facing feature, the parser/codegen mechanism (function
names, AST node types, helpers), any new stub-header functions, and whether it
moved an item out of "out of scope".

## Step 3 — Gather current metrics

The status snapshot has a metrics table. Collect fresh numbers:

```bash
ls src/*.c | wc -l                       # source files
ls include/*.h | wc -l                   # header files
cat src/*.c include/*.h | wc -l          # total LOC
git rev-list --count HEAD                # commits
git rev-parse --short HEAD               # HEAD hash for the dateline
ls test/valid/*.c | wc -l                # valid tests
ls test/invalid/*.c | wc -l              # invalid tests
ls -d test/integration/*/ | wc -l        # integration (exclude non-test dirs e.g. docs/)
ls test/print_ast/*.c | wc -l
ls test/print_tokens/*.c | wc -l
ls test/print_asm/*.c | wc -l
```

The authoritative total/breakdown is the **latest stage milestone's** test line;
prefer it when a raw count disagrees (integration counts can include a non-test
`docs/` subdir). Use today's date and the HEAD short hash in the new files'
datelines.

## Step 4 — Update the checklist (in place)

`docs/outlines/checklist.md` has per-stage `## Stage NN - Title` sections of
`- [x]` / `- [ ]` items, followed by a `---` separator and a `## TODO` section
grouped by area (Types, Declarations and Scope, Expressions, Statements,
Functions, Standard Library Headers, …).

- Insert a new `## Stage NN - Title` section for each new stage, **before** the
  `---` / `## TODO` marker, matching the existing tabbed-bullet style (tabs for
  nesting, sub-bullets for mechanism detail).
- For every TODO item a new stage now satisfies, flip `- [ ]` to `- [x]` and
  append `(Stage NN)`. If a feature is only partially done, keep it unchecked
  but add a checked sub-note (see the `volatile` and `va_arg` entries for the
  pattern).

## Step 5 — Write the new parse-tree document

Copy the most recent `docs/other/stage-*-parse-tree.md` as the template and
write `docs/other/stage-<NN>-parse-tree.md`. Preserve its four-section structure
(Program Structure / Statements / Expressions / Types and Declarators) and the
`└─►` / `├─►` call-depth notation. Update only the parser-facing changes:

- New or changed **parser functions / dispatch** (e.g. a new statement form, a
  relaxed lvalue check, an added declarator suffix).
- New **AST node types** and **helper functions** (`ast_clone`,
  `build_array_type_from_dims`, `eval_case_const_expr`, …).
- Tag each change with its stage number inline, as the template does.
- Refresh the **Key Design Points** list and the trailing **known gaps** note;
  move anything a new stage fixed out of the gaps list. Cross-check the gaps
  against `docs/self-compilation-report.md`.

## Step 6 — Write the new status and features snapshots

Copy the most recent `status/project-status-through-stage-*.md` and
`status/project-features-through-stage-*.md` as templates.

**Status** (`project-status-through-stage-<NN>.md`): keep the section layout
(Overview, Build & Test table, Language Features Implemented, Stage-by-Stage
Timeline, Recently Shipped, Out of Scope, Architecture, Process). Update the
metrics table, the stage count, the dateline, the timeline table (add the new
stages, mark the latest "(current)"), the per-feature prose, and the
Recently-Shipped narrative. Remove now-implemented items from Out of Scope.

**Features** (`project-features-through-stage-<NN>.md`): this is the dense
two-paragraph form — one "What works today:" paragraph and one "What's still out
of scope:" paragraph. Fold the new capabilities into the first paragraph
(bolding genuinely new headline features) and delete from the second paragraph
anything now implemented.

Keep the two snapshots consistent with each other and with the checklist.

## Step 7 — Verify and report

- Confirm the three new files were created and the checklist edited.
- Sanity-check the test totals add up to the milestone total.
- Do **not** delete the previous-stage snapshots — the `status/` history is
  intentionally cumulative.

Print a one-line-per-document summary, e.g.:

```
update-supplemental-docs: refreshed through stage <NN>
  docs/outlines/checklist.md            (+<k> stage sections, <m> TODOs checked)
  docs/other/stage-<NN>-parse-tree.md   (new)
  status/project-status-through-stage-<NN>.md   (new)
  status/project-features-through-stage-<NN>.md (new)
```

## Notes

- The checklist is edited in place; the other three are always new dated files.
- Stage milestones are the source of truth for "what shipped"; specs add detail.
- If the user names a specific stage range, honour it; otherwise default to
  "everything since the latest existing snapshot up to HEAD".
