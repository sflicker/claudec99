# ClaudeC99 stage 35 nested structs and arrays of structs

## Task
  - extend the compiler structs can contain other complete structs types as members
  - also allow arrays to have struct element types
  - Focus on layout, member access, lvalue/rvalue behavior and code generation

Examples
Nested structs
```C
        struct Inner {
            int z;
        };
        
        struct Outer {
            int x;
            int y;
            struct Inner I;
        }; 
  ```

Arrays with Struct Element type
```C
    struct Point {
        int x;
        int y;
    };
    
    struct Point points[10];
```

## Requirements
  - Support struct members whose type is another already define complete struct type
  - Correctly compute struct layout, size and alignment when struct members are nested structs.
  - Support chained member access such as `rect.origin.x`
  - Support arrays of structs such as `struct Point points[2]`
  - Support member access through array elements such as `points[0].x`
  - Support assignment to nested struct fields and fields of array elements
  - support reading nested struct fields and fields of array elements
  - Support `sizeof` for structs containing struct members and arrays of structs.

## Out of scope
  - Aggregate initializers
    - `struct Point p = { 1, 2 }`;
    - `struct Point points[] = { {1,2}, {3,4}};
  - Designated initializers
    - `struct POint p = { .x = 1, .y = 2` };`
  - Incomplete struct declarators
  - Recursive struct declarations
  - struct-by-value function parameters
  - struct return values
  - Full C99 initializer semantics

## Tests
Basic Nested struct
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
        struct Rect r;
        
        r.origin.x = 10;
        r.origin.y = 20;
        r.width = 3;
        r.height = 4;
        
        return r.origin.x + r.origin.y + r.width + r.height;   // expect 37
    }
```

Multiple nested struct members
```C
    struct Point {
        int x;
        int y;
    };
    
    struct Line {
        struct Point start;
        struct Point end;
    };
    
    int main() {
        struct Line line;
        
        line.start.x = 1;
        line.start.y = 2;
        line.end.x = 10;
        line.end.y = 30;
        
        return line.start.x + line.start.y + line.end.x + line.end.y;   // expect 33
    } 
```

Array of structs
```C
     struct Point {
         int x;
         int y;
     };
     
     int main() {
         struct Point points[2];
         points[0].x = 1;
         points[0].y = 2;
         points[1].x = 10;
         points[1].y = 20;
         return points[0].x + points[0].y + points[1].x + points[1].y;   //expect 33
     }
```
array of structs with nested structs
```C
    struct Point {
        int x;
        int y;
    };
    
    struct Rect {
        struct Point origin;
        int width;
        int helght;
    };
    
    int main () {
        struct Rect rects[2];
            rects[0].origin.x = 1;
    rects[0].origin.y = 2;
    rects[0].width = 3;
    rects[0].height = 4;

    rects[1].origin.x = 10;
    rects[1].origin.y = 20;
    rects[1].width = 30;
    rects[1].height = 40;

    return rects[0].origin.x
         + rects[0].origin.y
         + rects[0].width
         + rects[0].height
         + rects[1].origin.x
         + rects[1].origin.y
         + rects[1].width
         + rects[1].height;  // expect 110
}
```

`sizeof` nested struct
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
        return sizeof(struct Rect);   // expect 15
    }
```

Sizeof array of structs
```C
    struct Point {
        int x;
        int y;
    };
    
    int main() {
        struct Point points[3];
        return sizeof(points);    // expect 24
    }
```

Alignment test with mixed member sizes
```C
    struct Inner {
        char c;
        int x;
    };
    
    struct Outer {
        char a;
        struct Inner inner;
        char b;
    }
    
    int main() {
        resize sizeof(struct Outer);   // expect <??> depends on alignment used in implementation but likely 16. could also be 7 if no padding is used.
    }
```

## Invalid Tests
incomplete structs are not support yet, reject this
```C
    struct Outer {
        struct Missing m;   // INVALID
    };
    
    int main() {
        return 0;
    };
```

No nested member
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
        struct Rect r;
        return r.origin.z;       // INVALID;
    }
```

Array element must be indexed before field access
```C
    struct Point {
        int x;
        int y;
    };
    
    int main() {
        struct Point points[2];
        return points.x;    // invalid
    }
```

**implementation notes**
The main codegen rule is that nested member access should keep accumulation an address
For `rects[i].origin.y`

the address should be conceptually:
```C
    address(rects)
    + 1 * sizeof(struct Rect)
    +offset(Rect.origin)
    +offset(Point.y)
```
if sizeof(struct Rect) = 16
offset(Rect.origin) = 0
offset(Point.y) = 4

then
address = base + 1*16 +0 + 4;  