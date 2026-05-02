# ClaudeC99 Stage Transcript Format (v1)

## Purpose

This document defines the required format for generating a stage transcript (implementation report) after completing a stage.

The transcript is a structured, human-readable record of:

* what was implemented
* how it was implemented
* verification results
* high-level session flow

This is **not** a raw chat log. It is a clean, technical summary.

---

## Stage Label

The variable `STAGE_LABEL` must be derived from the spec filename (see SKILL.md rules).

Examples:

* `stage-01`
* `stage-07-02`
* `stage-08-01-03`

---

## Output File Naming

The transcript file must be named:

```
<STAGE_LABEL>-transcript.md
```

Example:

```
stage-07-03-transcript.md
```

---

## Title

Format:

```
# <STAGE_LABEL> Transcript: <Stage Title>
```

Rules:

* `<STAGE_LABEL>` must match the derived value exactly
* `<Stage Title>` is a short human-readable description (from spec or inferred)
* Do not change capitalization or structure

Example:

```
# stage-07-03 Transcript: Increment and Decrement Operators
```

---

## Required Sections (in exact order)

### 1. Summary

* 1–3 paragraphs of concise prose
* Must describe:

    * what feature was added
    * key semantics or behavior
    * any constraints or limitations
* No bullet points

---

### 2. Changes Made

Organized into numbered steps.

#### Step Format

```
### Step N: <Component Name>
```

Rules:

* Steps must be sequential starting from 1
* Each step corresponds to a logical implementation phase
* Typical components:

    * Tokenizer
    * Parser
    * AST
    * Code Generator
    * Tests

#### Content Rules

* Use bullet points
* Each bullet must describe a concrete change
* Avoid vague descriptions like “updated logic”

Example:

```
### Step 1: Tokenizer

- Added TOKEN_INCREMENT and TOKEN_DECREMENT to TokenType
- Updated '+' branch to check for '++' before '+='
```

---

### 3. Final Results

Must include:

* Build status
* Test results
* Regression status

Example:

```
All 69 tests pass (62 existing + 7 new). No regressions.
```

---

### 4. Session Flow

A concise numbered list describing the workflow.

Format:

```
## Session Flow

1. Read spec and supporting files
2. Reviewed relevant code
3. Produced summary and plan
4. Implemented changes by step
5. Built and ran tests
6. Recorded final results
```

Rules:

* 5–10 steps
* High-level only (not every micro action)
* Must reflect actual workflow

---

## Optional Sections

These may be included **only if relevant**.

### Plan

* Only if a planning phase occurred
* Must appear before "Changes Made"

### Tests

* Detailed breakdown of test cases added or modified
* Tables are allowed

### Notes

* Design decisions, constraints, or clarifications

### Commit

* Include commit hash if available

---

## Code Blocks

* Use fenced code blocks with language tags when appropriate
* Keep examples concise and relevant

Example:

```asm
mov eax, [rbp - offset]
add eax, 1
mov [rbp - offset], eax
```

---

## Formatting Rules

* Use Markdown
* Section names must match exactly
* Section order must not change
* Do not omit required sections
* Do not rename sections
* Use consistent indentation
* Keep formatting clean and readable

---

## Strict Rules

* Do not invent or guess STAGE_LABEL
* Do not modify STAGE_LABEL format
* Do not summarize steps — list concrete changes
* Do not include internal reasoning
* Do not include conversational text

---

## Summary of Structure

```
# <STAGE_LABEL> Transcript: <Stage Title>

## Summary

## Changes Made
### Step 1: ...
### Step 2: ...

## Final Results

## Session Flow

## (Optional Sections)
```

---

## End of File
