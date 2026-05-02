# ClaudeC99 Stage 15-04

## Task
  - Complete and verify character literal escape-sequence support and invalid literal diagnostics
  - Earlier Stage 15 work already aded character literal tokenization, AST Support, type handling and codegen.
  - This stage should avoid unnecessary rewrites and should only fill gaps in the current implementation.

## Requirements

### 1. Verify supported simple escapes

Character literals should support the standard simple C escape sequences:

```C
    '\a'   // alert/bell, value 7
    '\b'   // backspace, value 8
    '\f'   // form feed, value 12
    '\n'   // newline, value 10
    '\r'   // carriage return, value 13
    '\t'   // horizontal tab, value 9
    '\v'   // vertical tab, value 11
    '\\'   // backslash, value 92
    '\''   // single quote, value 39
    '\"'   // double quote, value 34
    '\?'   // question maek, value 63
    '\0'   // null character, value 0
```
If any of these already work, leave them alone, add only the missing ones

### 2. Verify invalid literal diagnostics

The compiler should reject malformed character literals with useful diagnostics

Invalid cases to verify:

```C
     ''   // empty character literal
     'ab'  // multi-character literal, out of scope
     '\q'  // unknown escape sequence
     '\    // unterminated escape sequence
     '     // unterminated character literal
```
Also reject a raw newline inside a character literal

Suggested diagnostic messages:
```C
    empty character literal
    multi-character literals are not supported
    unknown escape sequence in character literal
    unterminated escape sequence in character literal
    unterminated character literal
    newline in character literal
```

Exact wording does not need to match the strings if the existing diagnostics are already clear and tests follow the project's current style.

### Out of scope

```C
    '\123'    // general octal escape
    '\x41'    // hex escape
    'ab'      // multi-character constants
    L'A'      // wide character literals
    u'A'
    U'A'
```

The special '\0' escape remains supported as the null character, but general octal escapes are still out of scope

### Tests
Add or verify valid tests for the full simple-escape set
```C
    int main() { return '\a'; }  // return 7
    int main() { return '\b'; }  // return 8
    int main() { return '\f'; }  // return 12
    int main() { return '\n'; }  // return 10
    int main() { return '\r'; }  // return 13
    int main() { return '\t'; }  // return 9
    int main() { return '\v'; }  // return 11
    int main() { return '\\'; }  // return 92
    int main() { return '\''; }  // return 39
    int main() { return '\"'; }  // return 34
    int main() { return '\?'; }  // return 63
    int main() { return '\0'; }  // return 0
```

Add invalid tests for
```C
   int main() { return ''; }
   int main() { return 'ab'; }
   int main() { return '\q'; }
   int main() { return '\'; }
```

For newline-in-character-literal, create a source file that literally contains a newline between the opening and closing quote
```C
    int main() {
        return '
';
```

### Documentation

In addition to regular documentation tasks. Create in the *status* directory similar to the existing files
- project-features-through-stage-15-04.md
- project-status-through-stage-15-04.md

Leave the existing status documents alone. Create new ones that cover stages 1 to 15-04 using the existing files as templates.
