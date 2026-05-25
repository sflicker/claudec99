# Stage 70.02 Kickoff: Source Position Tracking in Tokens

## Summary of the Spec

Stage 70.02 adds source position tracking (line and column numbers) to every token produced by the lexer. Each token will track:

1. **Line number** (1-based): The line where the token starts
2. **Column number** (1-based): The column where the token starts
3. **Source file reference**: A pointer to the File object representing the source file the token came from

The lexer maintains current file, line, and column state as it processes input. When creating a token, it records the current position at the first character of the token. When the preprocessor includes a file, the lexer saves current file/line/col, resets to the new file with line=1 and col=1, and restores the saved position after the include is complete.

The `--print-tokens` output format is enhanced with a new position column added immediately after the index column, displayed as `[<line>:<col>]` with dynamically-sized widths and leading zeros. The width for both line and column is determined at runtime based on the maximum values found in the entire token stream.

## Required Tokenizer Changes

1. **Token struct modifications** (`include/token.h`):
   - Add `line` field (int): 1-based line number
   - Add `col` field (int): 1-based column number
   - Add `file` field (SourceFile*): pointer to source file

2. **SourceFile struct** (`include/token.h`):
   - Create new `SourceFile` struct to hold file path and metadata
   - Each token maintains reference to its source file

3. **Lexer struct modifications** (`include/lexer.h`):
   - Add `line` field (int): current line number being processed
   - Add `col` field (int): current column number being processed
   - Add `file` field (SourceFile*): current file being processed
   - Add file pool/arena for allocating SourceFile structs

4. **Lexer initialization** (`include/lexer.h` and `src/lexer.c`):
   - Update `lexer_init()` signature to accept initial `SourceFile*` parameter
   - Add `lexer_free()` function to clean up file pool and other resources
   - Initialize line=1, col=1 at lexer start

5. **Lexer position tracking** (`src/lexer.c`):
   - Track column increments during normal character consumption
   - Reset column to 1 and increment line when encountering newline characters
   - Track position during whitespace skipping (spaces and tabs increment col, newlines increment line)
   - Set token position to current line/col at first character of token
   - Handle `\x01` markers (SOH sentinel bytes) injected by preprocessor:
     - Format: `\x01<line>:<path>\n`
     - Parse to extract new line number and file path
     - Create or reuse SourceFile for the path
     - Update lexer's current file/line/col state

## Required Parser Changes

None. The parser does not need to change; it consumes tokens with position information already populated.

## Required AST Changes

None. The AST does not need to change; position information is stored in tokens, not propagated through AST nodes.

## Required Code Generation Changes

1. **Token printing** (`src/compiler.c`):
   - Calculate max line and max column across all tokens
   - Determine width needed for line and column numbers (using leading zeros)
   - Update `--print-tokens` output format to include position column as `[<line>:<col>]` after index
   - Maintain alignment across all token rows

## Required File Modifications

### Modified Files

1. **include/token.h**:
   - Define `SourceFile` struct with file path and metadata
   - Add `line`, `col`, `file` fields to `Token` struct

2. **include/lexer.h**:
   - Add `line`, `col`, `file` fields to `Lexer` struct
   - Add file pool/arena field to `Lexer`
   - Update `lexer_init()` signature to accept `SourceFile*`
   - Add `lexer_free()` declaration

3. **src/lexer.c**:
   - Implement position tracking during character consumption
   - Track line/col changes when skipping whitespace
   - Parse `\x01<line>:<path>\n` markers from preprocessor
   - Set token position at token creation
   - Implement `lexer_free()` function

4. **src/preprocessor.c**:
   - Inject `\x01<line>:<path>\n` markers at `#include` boundaries
   - Markers inform lexer of file/line transitions

5. **src/compiler.c**:
   - Update all `lexer_init()` calls to pass initial `SourceFile*`
   - Add calls to `lexer_free()` when lexer is no longer needed
   - Enhance `--print-tokens` output with position column
   - Calculate max line/col widths dynamically

6. **test/print_tokens/*.expected**:
   - Regenerate all 99 expected output files to include position information
   - Format: `<index> [<line>:<col>] <token_type> <token_value>`

## Implementation Notes

- **Marker format**: The preprocessor uses `\x01` (SOH, ASCII 1) as a sentinel byte to communicate file/line changes to the lexer. This byte cannot appear in valid C source, making it a safe choice for in-band signaling.
- **Position at first character**: Token position is set when the lexer encounters the first character of the token, before any multi-character token logic processes it.
- **Width calculation**: The width for line and column numbers must accommodate the maximum values found across all tokens in the stream. This requires a pass to collect all tokens first, then a second pass to print them with correct widths. Alternatively, a two-pass approach or pre-calculation of max values is needed.
- **File pool**: The lexer maintains a pool/arena of SourceFile structs to avoid duplicate file entries and enable safe pointer references from tokens.
- **No grammar changes**: This stage does not modify the C grammar; it only adds metadata to existing tokens.

## Out-of-Scope

- Using token positions in error messages (deferred to future stages)
- AST node position tracking (tokens carry positions; AST nodes do not)

## Test Plan

### Unit Tests

1. **Basic position tracking**: Verify simple single-token and multi-token inputs record correct line/col
2. **Newline handling**: Verify line counter increments and column resets at newlines
3. **Whitespace handling**: Verify spaces and tabs increment column correctly
4. **Multi-line input**: Verify tokens on different lines have correct line numbers
5. **Include boundaries**: Verify `\x01` markers trigger file/line transitions
6. **Print tokens format**: Verify `--print-tokens` output includes correctly formatted position column

### Integration Tests

1. **print_tokens test suite**: Regenerate and validate all 99 existing test cases with new position column
2. **Width calculation**: Test that position column widths adjust correctly for files with varying line/col counts
3. **Multiple includes**: Test position tracking across nested includes
4. **Compatibility**: Verify existing functionality is unaffected by new fields

## Known Constraints

1. The marker format `\x01<line>:<path>\n` is an implementation detail not formally specified; care must be taken in preprocessor injection and lexer parsing.
2. Two-pass processing (calculate widths, then print) may be needed for `--print-tokens` output, adding complexity.
3. File pool management requires careful memory allocation to ensure SourceFile pointers remain valid for the lifetime of tokens.
4. The lexer's position tracking must correctly handle all whitespace characters and boundary conditions.
