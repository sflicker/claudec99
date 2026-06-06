# ClaudeC99 - 95-01 dynamic capacity cleanup

## Goal
Replace the fixed-sized compiler tables/buffers with growable storage so self-hosting or other compiler users no
longer fails because of arbitrary likes

## Part 1 - inventory fixed limits
Find every remaining fixed-capacity table and classfy it.
For each record
  - name
  - current max
  - file/module
  - what happens on overflow
  - whether entries are referenced by pointers elsewhere
  - whether entries can savely move after realloc
  - priority for self hosting plan

This inventory should be persisted as an md file so future stages can act upon it and mark off finished items

## Out of scope
  No code changes should be made this stage should only generate an artififact that inventories
  the current state of fixed limits within the source code.

