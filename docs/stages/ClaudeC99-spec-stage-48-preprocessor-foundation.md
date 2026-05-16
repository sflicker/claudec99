# Claudec99 Stage 48 - Preprocessor foundation

## Task
 Add the first standalone preprocessing pass for Claudec99
 
The stage establishes the preprocessor as a separate module that runs before the normal tokenizer.
It does not implement includes, macros or conditional processing yet

Pipeline:
```text
Raw source file
    ↓
preprocessor
    ↓
preprocessed source text
    ↓
normal tokenizer
    ↓
Parser
    ↓
AST/semantic analysis
    ↓
codegen    
```

## Requirements
**Preprocessor module**
Add:
```C
    include/proprocessor.h
    src/proprocessor.c
```

New FLow:
```C
    read source file->preprocess->tokenize->parse->codegen
```

### Comments
Comments are currently being handled by the tokenizer. This logic should be removed from the tokenizer.
Comments logic should be added to the preprocessor. Comments should be replace by whitespace not
simply removed. This includes both line and block style.

Example 
```C
   x = 4/* comment */2;   →    x = 4 2;    // comment replace with 1 space
```

### Line Splicing
Handle backslash-newline before comment removal and tokenization
Example
```C
    int main() {
        return 42\
    2;
    }
```
Should be replaced with
```C
   int main() {
       return 42;
   }
```

### Directive recognition
Recognize preprocessor directive lines beginning with optional white space followed by `#`

For this stage, cleanly reject all redirects with a useful error message (e.g. unsupported directive).

Examples that should produce clean un unsupported directive errors.

```C
#define X 42
```

```C
#include "x.h"
```

```C
#ifdef X
#endif
```

## Out of scope
  - `#include`
  - `define`
  - macro expansion
  - function like macros
  - `#if`
  - `#ifdef`
  - `#ifndef`
  - `#else`
  - `#elif`
  - `#endif`
  - predefined macros
  - `__FILE__`
  - `__LINE__`
  - include guards
  - full preprocessing token model
  - token-stream preprocessor

## Valid Tests

Line Comment
```C
// this is a comment
int main() {
    return 42;  // EXPECT: status code 42
}
```

End of line comment
```C
    int main() {  // this is a comment
        return 42;  EXPECT: 42
    }
```

Block comment inside expression
```C
int main() {
    return /*comment*/ 42;    // expect: 42
}
```

Multiline block comment
```C
    int main() {
        /*
            comment
        */
        return 42;    // expect: 42
    }
```

comment between tokens
```C
    int main() {
        int x;
        x = 4/*comment*/ + 2;
        return x;    // EXPECT 6
    }
```

Line splicing
```C
    int main() {
        return 4\
    2;           // EXPECT: 42
    }
```

Line splicing across identifier
```C
    int main() {
        int fo\
        o;
        foo = 7;
        return foo;   // EXPECT 7
    }
```

### Invalid Tests
Comment must not join tokens
```C
    int main() {
        int x;
        x = 4/*comment*/2;   //INVALID THIS SHOULD TRIGGER A COMPILER ERROR
        return x;
```

Unsupported define
```C
#define X 42        // INVALID FOR THIS STAGE

int main() {
    return X;
}
```

Unsupported include
```C
#include "helper.h"   // INVALID FOR THIS STAGE
int main() {
    return X;
}

```

Unterminated block comment
```C
    int main() {
        /*
        return 42;
    }
```