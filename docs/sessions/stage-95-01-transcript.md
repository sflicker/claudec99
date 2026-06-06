# stage-95-01 Transcript: Fixed-Capacity Inventory

## Summary

Stage 95-01 is a documentation-only stage with no compiler code changes. Its purpose was to produce a comprehensive inventory of all fixed-capacity tables and buffers in the ClaudeC99 compiler source, classifying each with metadata to enable future dynamic-allocation implementation work. The stage involved a systematic audit of the compiler source code to identify every array, stack variable, and macro-defined limit used as a fixed capacity boundary. For each item, metadata was collected including current capacity, file location, overflow behavior, pointer aliasing characteristics, realloc safety, and priority ranking. The inventory is organized by subsystem (AST, Parser, Codegen, Preprocessor, Type System, String Name Buffers) and includes a priority ranking summary. Key findings include three items marked HIGH priority for immediate action (MAX_BREAK_DEPTH has no bounds check and causes silent buffer overflow; PARSER_MAX_STRUCT_FIELDS uses a hardcoded literal instead of the constant causing silent field loss; PARSER_MAX_TYPEDEFS and PARSER_MAX_FUNCTIONS are frequently saturated). Three subsystems were found to already have dynamic storage: MacroTable, GrowBuf, and ASTNode.children (converted in stage 92). The inventory is designed as a living document where future stages will mark items complete as they migrate to growable storage.

## Changes Made

### Step 1: Source Audit — Identify All Fixed-Capacity Items

- Systematically searched all compiler source files for array declarations with fixed dimensions
- Identified macro-defined constants used as capacity limits in `include/constants.h`
- Located all overflow checks, bounds tests, and error messages related to capacity
- Collected 23 fixed-capacity items across all compiler subsystems
- Classified items by subsystem: Parser (8 items), Codegen (9 items), String buffers (1 item), Type System (1 item), AST (1 item), Preprocessor (2 items), Already Dynamic (3 items)

### Step 2: Metadata Collection — Overflow Behavior

- For each item, determined what happens when capacity is exceeded
- Categorized overflow behaviors into three types: hard `PARSER_ERROR` / `compile_error`, silent data loss/truncation, and unchecked buffer overflow
- Key discovery: MAX_BREAK_DEPTH has **no check** before writing; exceeding 32 nesting levels silently corrupts adjacent `CodeGen` fields
- Key discovery: PARSER_MAX_STRUCT_FIELDS uses hardcoded literal `64` instead of `PARSER_MAX_STRUCT_FIELDS` constant; fields beyond 64 are silently dropped
- Most parser and codegen limits produce hard errors; preprocessor limits are mostly recursion counters with soft errors

### Step 3: Metadata Collection — Pointer Aliasing and Realloc Safety

- For each item, traced whether returned pointers escape the immediate call frame
- Classified as "Yes" (pointers are returned but used only locally), "No" (pointers escape or entries are never referenced by pointer), "N/A" (not heap-allocated or recursion counter)
- For heap-allocated arrays, determined whether entries can safely move after `realloc`
- Key finding: Most parser arrays (funcs, globals, typedefs, etc.) return local-only pointers; safe for realloc
- Most codegen arrays are embedded in larger structs or have complex layout (e.g., SwitchCtx with nested arrays); less safe for realloc
- String name buffers (char[MAX_NAME_LEN]) embedded in various AST/signature structs; cannot move without major refactoring

### Step 4: Metadata Collection — Priority Ranking

- Ranked each item as HIGH, MEDIUM, or LOW priority for dynamic-capacity conversion
- **HIGH priority:** MAX_BREAK_DEPTH (silent overflow), PARSER_MAX_STRUCT_FIELDS (silent data loss), PARSER_MAX_TYPEDEFS (tightest commonly-hit limit), PARSER_MAX_FUNCTIONS (frequently exceeded in large translation units)
- **MEDIUM priority:** Parser globals/enum consts, Codegen locals/globals/string pool, various struct tag limits; all have hard errors but are less frequently saturated
- **LOW priority:** Type system limits, recursion depth counters, stack-local arrays (MAX_ARRAY_DIMS, MAX_CALL_LAYOUT_ITEMS, MAX_COND_DEPTH), less-common limits like MAX_USER_LABELS and MAX_SWITCH_LABELS

### Step 5: Document Already-Dynamic Subsystems

- Identified three subsystems that already use dynamic storage:
  - `MacroTable` in `src/preprocessor.c` — heap-allocated, grows by doubling via `realloc`, fully dynamic since inception
  - `GrowBuf` in `src/preprocessor.c` — output buffer, grows by doubling via `realloc`, fully dynamic since inception
  - `ASTNode.children` — converted from fixed array to dynamically growing pointer array (initial capacity `AST_MAX_CHILDREN`, doubles on overflow) in stage 92
- These do not require action in future stages; documented for completeness

### Step 6: Generate Inventory Document

- Produced `docs/fixed-capacity-inventory.md` with all 23 items organized by subsystem
- Included comprehensive metadata columns: Name, Max, Module, On Overflow, Ext Ptr Refs, Safe Realloc, Priority, Status
- Added summary table grouping items by priority (HIGH/MEDIUM/LOW)
- Documented all findings including critical bugs (MAX_BREAK_DEPTH unchecked write, PARSER_MAX_STRUCT_FIELDS hardcoded literal)
- Designed Status column to track future progress; initialized all items to PENDING

### Step 7: Specification Review and Notes

- Reviewed the stage spec (`docs/stages/ClaudeC99-spec-stage-95-01-dynamic-capcity-cleanup.md`) and identified five cosmetic typos:
  1. Filename contains "capcity" instead of "capacity"
  2. Spec text: "arbitrary likes" should be "arbitrary limits"
  3. Spec text: "classfy" should be "classify"
  4. Spec text: "savely" should be "safely"
  5. Spec text: "artififact" should be "artifact"
- Determined that all typos are cosmetic and the intent is unambiguous; no spec corrections were required
- Noted these as documentation issues for reference but did not alter the spec

## Final Results

**Deliverable:** `docs/fixed-capacity-inventory.md` — comprehensive inventory of 23 fixed-capacity items in the ClaudeC99 compiler.

**Subsystem Summary:**

| Subsystem | Item Count | Key Findings |
|-----------|-----------|---|
| Parser | 8 items | PARSER_MAX_FUNCTIONS (256), PARSER_MAX_TYPEDEFS (64 — tightest), PARSER_MAX_STRUCT_FIELDS (hardcoded literal bug, silent data loss) |
| Codegen | 9 items | MAX_BREAK_DEPTH (no bounds check — silent overflow), MAX_LOCALS/MAX_GLOBALS (256 each), MAX_STRING_LITERALS (2048 after stage 92 raise) |
| Type System | 1 item | FUNC_TYPE_MAX_PARAMS (16, embedded in struct) |
| String Name Buffers | 1 item | MAX_NAME_LEN (256 bytes, embedded in multiple structs, silent truncation) |
| AST | 1 item | AST_MAX_CHILDREN (already dynamic since stage 92) |
| Preprocessor | 2 items | MAX_INCLUDE_DEPTH (64, recursion counter), MAX_COND_DEPTH (64, local stack) |
| Already Dynamic | 3 items | MacroTable, GrowBuf, ASTNode.children — no action needed |

**Priority Summary:**

- **HIGH (4 items):** MAX_BREAK_DEPTH, PARSER_MAX_STRUCT_FIELDS, PARSER_MAX_TYPEDEFS, PARSER_MAX_FUNCTIONS
- **MEDIUM (6 items):** PARSER_MAX_GLOBALS, MAX_LOCALS, MAX_GLOBALS, PARSER_MAX_STRUCT_TAGS, MAX_STRING_LITERALS, PARSER_MAX_ENUM_CONSTS
- **LOW (13 items):** MAX_NAME_LEN, FUNC_TYPE_MAX_PARAMS, FUNC_MAX_PARAMS, MAX_CALL_LAYOUT_ITEMS, PARSER_MAX_ENUM_TAGS, PARSER_MAX_UNION_TAGS, MAX_SWITCH_DEPTH, MAX_SWITCH_LABELS, MAX_USER_LABELS, MAX_LOCAL_STATICS, MAX_INCLUDE_DEPTH, MAX_COND_DEPTH, MAX_ARRAY_DIMS

**Critical Bugs Identified (not fixed in this stage, documented for future action):**

1. **MAX_BREAK_DEPTH overflow:** `cg->break_stack[cg->break_depth]` written at 4 sites (while, do-while, for, switch) without bounds check. Exceeding 32 nesting levels silently corrupts adjacent `CodeGen` fields.
2. **PARSER_MAX_STRUCT_FIELDS silent data loss:** Overflow check at `src/parser.c` lines 397, 590 uses hardcoded literal `if (n_fields < 64)` instead of `PARSER_MAX_STRUCT_FIELDS`. Fields beyond 64 are silently dropped and struct size/alignment become incorrect.

**Test Status:** No tests added or run — documentation-only stage with no code changes.

## Session Flow

1. Read the stage spec to understand the goal: comprehensive inventory of all fixed-capacity items in the compiler source.
2. Reviewed recent milestone and transcript examples (stage 94) to understand style and structure.
3. Planned the audit approach: systematic search by subsystem, metadata collection, priority ranking, documentation generation.
4. Audited compiler source files to identify all fixed-capacity arrays, stack variables, and macro limits:
   - `include/constants.h` and `include/codegen.h` for most constants and arrays
   - `include/parser.h` for Parser struct members
   - `src/parser.c`, `src/codegen.c`, `src/preprocessor.c` for implementations and overflow checks
5. Collected metadata for each item: capacity, file location, overflow behavior, pointer aliasing, realloc safety.
6. Identified critical bugs during analysis: MAX_BREAK_DEPTH unchecked write, PARSER_MAX_STRUCT_FIELDS hardcoded literal.
7. Classified items into subsystems and priority tiers (HIGH/MEDIUM/LOW).
8. Documented three already-dynamic subsystems: MacroTable, GrowBuf, ASTNode.children.
9. Generated `docs/fixed-capacity-inventory.md` with comprehensive metadata organized by subsystem and priority.
10. Reviewed spec for issues and noted five cosmetic typos (not corrected; intent unambiguous).
11. Confirmed no code changes, no version bump, no test run required.
12. Produced kickoff and (now) milestone and transcript artifacts.

## Notes

- **Documentation-only stage:** No code changes, no version bump, no self-host test phase, no README or grammar updates.
- **Living document:** The inventory is designed as a reference that future stages will update as items are converted to dynamic storage; Status column initialized to PENDING for all items.
- **Spec issues:** Five cosmetic typos identified in the spec filename and body ("capcity", "likes", "classfy", "savely", "artififact"); all are obvious corrections and the intent is unambiguous. These were noted but not fixed per stage design (documentation-only).
- **Critical bugs found:** MAX_BREAK_DEPTH and PARSER_MAX_STRUCT_FIELDS both present silent data corruption/loss risks and are marked HIGH priority for future stages.
- **Pointer aliasing analysis:** Most parser arrays are safe for realloc (pointers returned but used only locally); codegen arrays and string name buffers are less safe due to embedding in larger structs.
