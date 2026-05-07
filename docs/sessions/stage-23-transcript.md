# stage-23 Transcript: Storage Class Basics

## Summary

Stage 23 added support for file-scope `extern` and `static` storage class specifiers, enabling control over function and object linkage. The implementation distinguishes declarations (extern, no storage emitted) from definitions (implicit external or static linkage); validates that linkage cannot be mixed (static and extern), that duplicate static definitions are rejected, and that storage class specifiers are confined to file scope. Codegen now conditionally emits the `global` NASM directive only for non-static functions, and skips storage allocation for extern-only objects.

## Changes Made

### Step 1: Tokenizer

- Added TOKEN_EXTERN and TOKEN_STATIC to TokenType enum in include/token.h
- Updated lexer to recognize "extern" and "static" keywords in src/lexer.c

### Step 2: AST

- Added StorageClass enum (SC_NONE=0, SC_EXTERN=1, SC_STATIC=2) to include/ast.h
- Added `storage_class` field to ASTNode struct to track storage class for all nodes
- Added `storage_class` field to FuncSig struct in include/parser.h
- Added `storage_class` field to GlobalObjSig struct in include/parser.h

### Step 3: Parser

- Updated parse_external_declaration in src/parser.c to parse optional storage class specifier before type specifier
- Added validation to reject extern with initializer (semantic error at parse time)
- Added validation to reject multiple storage class specifiers (e.g., extern static)
- Added block-scope validation in parse_statement to reject storage class in local declarations
- Updated parser_register_global to track storage class and validate linkage consistency: reject static/extern conflicts, reject duplicate static definitions, allow repeated extern declarations
- Updated parser_register_function to validate storage class consistency: reject static followed by non-static function, prevent static/extern conflicts

### Step 4: Codegen

- Modified global registration pass to skip extern-only declarations (no storage emitted)
- Updated code generator to emit `global funcname` NASM directive only for non-static functions
- Static objects and functions remain local to the translation unit

### Step 5: Tests

- Added 6 valid tests: extern object followed by definition, repeated extern declarations, extern function prototype, static global object, static function, static counter with persistence
- Added 5 invalid tests: conflicting linkage (static then extern), extern with initializer, static then non-static function mismatch, duplicate static definition, block-scope storage class

## Final Results

All 668 tests pass (657 existing + 11 new). No regressions. Build successful.

## Session Flow

1. Read spec (ClaudeC99-spec-stage-23-storage-class-basics.md) and identified requirements
2. Reviewed existing token, AST, and parser headers to understand extension points
3. Added TOKEN_EXTERN and TOKEN_STATIC to lexer
4. Extended AST with StorageClass enum and storage_class field to ASTNode
5. Updated FuncSig and GlobalObjSig structs with storage_class field
6. Modified parse_external_declaration to recognize and parse storage class specifier
7. Implemented linkage validation in parser_register_global and parser_register_function
8. Updated codegen to conditionally emit global directive and skip extern-only objects
9. Created 11 new tests (6 valid, 5 invalid) covering all major cases
10. Ran full test harness; verified all 668 tests pass
