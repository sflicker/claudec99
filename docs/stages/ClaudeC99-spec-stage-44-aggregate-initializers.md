# ClaudeC99 Stage 44 - aggregate initializers

## Goals
  - struct object initializers
  - array of structs initializers
  - nested aggregate initializers
  - file scope aggregate initializers
  - local aggregate initializers

## Tests
Some of these tests require a companion .expected file with the expect output along with linking.
The ones requiring output checks have "// EXPECTED OUTPUT:  "   in the comments

Local struct initializers
```C
    struct Point {
        int x;
        int y;
    };
    
    int main() {
        struct Point p = {10,20};
        return p.x + p.y;  // expect 30
    }
```

File scope struct initializer
```C
    struct Point {
        int x;
        int y;
    };
    
    struct Point p = {3,4};
    
    int main() {
        return p.x + p.y;  // expect 7
    }
```

Local array of structs
```C
    struct Point {
        int x;
        int y;
    };
    
    int main() {
    struct Point points[2] = {
        {1,2},
        {10,20}
    };
    
    return points[0].x + points[0].y + points[1].x + points[1].y; // expect 33
}
```

file scope array of structs
```C
    struct Point {
        int x;
        int y;
    };
    
    struct Point points[] = {
        {1, 2},
        {10, 20},
        {100, 200 }
    };
    
    int main() {
        return points[0].x + points[1].x + points[2].x; // expect 111
    } 
```

Nested struct initializer
```C
    struct Point {
        int x;
        int y;
    };
    
    struct Rect {
        struct Point origin;
        int width;
        int height;
    };
    
    int main() {
        struct Rect r = { {1,2}, 3, 4};
        
        return r.origin.x + r.origin.y + r.width + r.height; // expect 10
    }        
```

compile table style initializer

```C

    int puts(char *);
    
    struct TokenMapEntry {
        char * name;
        int kind;
    };
    
    struct TokenMapEntry keywords[] = {
        { "if" , 1 },
        { "else", 2 },
        { "while", 3 },
        { "return", 4} };
        
    int main() {
        puts(keywords[3].name);   // EXPECT OUTPUT: "return"
        return keywords[3].value;    // expect 4;
    }
```

string field access from table
```C
    int puts(char*);
    
    struct TokenMapEntry {
        char * name;
        int kind;
    };
    
    struct TokenMapEntry keywords[] = {
        {"if", 1},
        {"else", 2}
    };
    
    int main() {
        puts(keywords[0].name);   // EXPECT OUTPUT: "if"
        return keywords[1].kind;
    }
```

## Invalid Tests
too many struct initializers
```C
    struct Point {
        int x;
        int y;
    };
    
    int main() {
        struct Point p = {1,2,3};    // reject
    }
```

too many elements
```C
    int values[2] = {1,2,3};     // reject
    
    int main() {
        return 0;
    }
```

incompatible field initializer
```C
    struct Point {
        int x;
        int y;
    };
    
    int main() {
        struct Point p = {"bad", 2};  // reject
        return 0;
    }
```

```C
    struct Entry {
        char * name;
        int value;
    };
    
    int main() {
        struct Entry e = {1,2};  // reject
        return 0;
    }
```