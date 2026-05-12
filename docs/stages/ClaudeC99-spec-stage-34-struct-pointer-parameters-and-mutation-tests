# Stage 34 Struct pointer parameter and mutation tests

## Task
  - Supporter pointers to structs as function arguments
  - add additional struct mutation tests

  Main Goal
  ```C
      struct Point {
          int x;
          int y;
      };

      int move(Struct Point *p, int dx, int dy) {
          p->x = dx;
          p->y = dy;
          return 0;
      }

      int main() {
          struct Point p;
          p.x = 10;
          p.y = 20;
          move(&p, 3, 4);
          return p.x + p.y;  // expect 37
      }
  ```
## Requirements
  1. functions may take pointers to named struct types
  2. `->` works on struct pointer parameters as both rvalue and lvalue
  3. `(*p).field` should work if dereference and `.` already exist
  4. passing &local_struct works
  5. passing &global_struct works
  6. mutations exist after the function returns
  7. type checking rejects incompatible struct pointer types
  8. reject invalid member access.

## Type Checking
   - ensure `->` requires pointer-to-struct
   - ensure `.` require struct object
   - ensure member lookup uses the pointed to struct type for ->

## codegen
   - load pointer parameter value
   - add member offset
   - emit load/store depending on rvalue/lvalue use

## function calls
   - verify &struct_object products pointer-to-that-struct
   - verify parameter compatibility for struct pointers

## Tests
  ```C
      struct Point {
          int x;
          int y;
      };

      int move(Struct Point *p, int dx, int dy) {
          p->x = dx;
          p->y = dy;
          return 0;
      }

      int main() {
          struct Point p;
          p.x = 10;
          p.y = 20;
          move(&p, 3, 4);
          return p.x + p.y;  // expect 37
      }
  ```

  Read through struct pointer
  ```C
      struct Point {
          int x;
          int y;
      };

      int sum_point(struct Point *p) {
          return p->x + p->x;
      }

      int main() {
          struct Point p;
          p.x = 11;
          p.y = 22;
          return sum_point(&p);  // expect 33
      }
  ```

 Mixed mutation and read
 ```C
     struct Counter {
         int value;
     };

     int inc(struct Counter *c) {
         c->value = c->value + 1;
         return 0;
     }

     int main() {
         struct Counter c;
         c.value = 40;
         inc(&c);
         int(&c);
         return c.value;  // expect 42
     }
 ```

 ### Invalid cases
 Missing field
 ```C
     struct Point {
        int x;
        int y;
     };

     int move(struct Point *p, int dx, int dy) {
         p->z;   //INVALID
         return 0;
     }

     int main() {
         struct Point p;
         p.x = 1;
         p.y = 1;
         move(&p);
         return 0;
     }
 ```

 P is a struct pointer not struct object
 ```C
     struct Point {
         int x;
         int y;
     };

     int move(struct Point p, int dx, int dy) {
         p.x = dx;    // INVALID
         p.y = dy;
         return 0;
     }

     int main() {
         struct Point p;
         p.x = 1;
         p.y = 1;
         move(&p);
         return 0;
     }

 ```
 p is struct object not pointer;
 ```C
     struct Point {
         int x;
         int y;
     };

     int main() {
         struct Point p;
         p->x = 10;    //INVALID
         p->y = 10;
         return 0;
     }

 ```