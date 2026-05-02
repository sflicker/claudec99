# Milestone Summary Format

The milestone summary should be consise and suitable for saving directly in `docs\milestones`

Do not produce a long implementation audit. Focus on what the stage completed, 
    Tha major areas touched, tests added summary, docs updated and final test result.

Use this structure

```markdown
# Milestone Summary

## <STAGE_DISPLAY>: <short stage title> - Complete

<STAGE_LABEL> ships <one-sentence summary of the completed feature>.
- Tokenizer: <brief tokenizer changes, if any>.
- Grammar/Parser: <brief grammar and parser changes, if any>.
- AST/Semantics: <brief AST/type-checking/semantics changes, if any>.
- Codegen: <brief code generation changes, if any>.
- Tests: <count and broad categories>. Full suite <pass-count>/<total-count> pass.
- Docs: <brief docs updated, if any>.
- Notes: <important spec issue, limitation, or follow-up, if any>.
```

## Guidelines
  - Keep the summary around 6-10 bullets
  - Avoid listing every individual test unless there are only a few.
  - Mention exact test total when available.
  - Mention generated artifacts only if the workflow asks for a completion note outside the saved milestone.
  - Prefer concise subsystem bullets over detailed step-by-step implementation history.
  - Do not include transcript-level detail.
