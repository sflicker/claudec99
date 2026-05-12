# ClaudeC99 stage 36 typedef for complete struct forms

## task
  - support typedef with struct
Examples
```C
   struct Point {
    int x;
    int y;
   };
 
   typedef struct Point Point;

   int main() {
       Point p;
       p.x = 10;
       p.y = 20;
       return p.x + p.y;
    }
  ```

```C
typedef struct Point {
    int x;
    int y;
} Point;

int main() {
    Point p;
    p.x = 3;
    p.y = 4;
    return p.x + p.y;
}
```

```C
    typedef struct Point {
      int x;
      int y;
    } Point;
    
    int main() {
        Point points[2];
        points[0].x = 1;
        points[0].y = 2;
        points[1].x = 10;
        points[1].y = 20;
        return points[0].x + points[0].y + points[1].x + points[1].y;
    }
```

Make appropriate changes to enable the above examples to work