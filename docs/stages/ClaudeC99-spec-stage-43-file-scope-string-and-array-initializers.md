# ClaudeC99 Stage 43 - File scope string and array initializers

## Goals
  - This stage implements file-scope initialization semantics and static data emission for selected initializer forms.
    - File-scope char array initialized from string literal
    - File-scope char * initialized from string literal
    - File-scope array of char * initialized from string literals
    - File-scope integer array initialized from constant expressions

## Out of Scope
  - struct aggregate initializers
  - nested aggregate initializers
  - designated initializers

## Tests
Some of these tests require a companion .expected file with the expect output along with linking.
The ones requiring output checks have "// EXPECTED OUTPUT:  "   in the comments

File-scope char array from string literal
```C
    int puts(char *s);
    
    char s[] = "abc";
    
    int main() {
        puts(s);     // EXPECTED OUTPUT: "abc"
        return 0;    // EXPECT EXIT CODE: 0
    }
```

File-scope pointer to string literal
```C
    int puts(char *);
    
    char *s = "abc";
    
    int main() {
        puts(s);    // EXPECTED OUTPUT: "abc"
        return 0;   // EXPECTED EXIT CODE: 0
    }
```

File-scope array of pointers to string literals
```C
    int puts(char *);
    
    char *names[] = {"if", "else", "while"};
    
    int main() {
        puts(names[1]);   // EXPECTED OUTPUT: "else"
        return 0;         // EXPECTED EXIT CODE: 0
    }
```

File-scope integer arrays
```C
    int values[] = {10,20,30};
    
    int main() {
        return values[0] + values[1] + values[2];  // expected 60
    }
```
