# ClaudeC99 Stage 33 struct assignment

## Task
  - allow whole struct objects to be copied
  
Example
```C
      struct Point {
          int x;
          int y;
      };
      
      int main() {
          struct Point c = { 3, 4};
          struct Point d;       // also support declaration initialization
                                // from another struct object like: 
                                // struct Point d = c;
          
          d = c;
          
          return d.x + d.y;   // expect 7
      } 
  ```

## Requirement
  - allow structs to be copied if the source and destination are the same struct type
  - reject assignment if the structs are different

## Implementation idea
  - struct assignment is a memory copy of sizeof(Struct T) bytes from source
    object to destination object.

## Tests
```C
      struct Point {
          int x;
          int y;
      };
      
      int main() {
          struct Point c = { 3, 4};
          struct Point d;       
          
          d = c;
          
          return d.x + d.y;   // expect 7
      } 
```

```C
      struct Point {
          int x;
          int y;
      };
      
      int main() {
          struct Point c = { 3, 4};
          struct Point d = c;       
          
          return d.x + d.y;   // expect 7
      } 
```

## Invalid Test
```C
    struct Point {
        int x;
        int y;
    };
    
    struct Other {
        int x;
        int y;
    };
    
    int main() {
        struct Point p = {1,2};
        struct Other q;
        
        q = p;     // INVALID
    }
```