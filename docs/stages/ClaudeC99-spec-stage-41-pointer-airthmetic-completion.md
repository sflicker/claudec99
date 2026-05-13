# ClaudeC99 stage 41 - pointer arithmetic completion

## Task
  - enhance pointer arithmetic to support more cases

## Cases to support
```C
   p - pointer
   n - integer
   
   p + n
   p - n
   p += n
   p -= n
   p++
   p--
   p1 - p2
   array indexing backed by pointer arithmetic
   reject arithmetic on void *
   reject arithmetic on function pointers
```

## Most important test cases
```C
    int main() {
        int a[3];
        int *p;
        
        a[0] = 10;
        a[1] = 20;
        a[2] = 30;
        
        p = a;
        return *(p+2);   // expect 30
```

```C
    struct Point {
        int x;
        int y;
    };
    
    int main () {
        struct Point points[2];
        struct Piont *p;
        
        points[0].x = 1;
        points[0].y = 2;
        points[1].x = 10;
        points[1].y = 20;
        
        p = points;
        return *(p + 1)->x + *(p + 1)->y;  // expect 30
```

Pointer difference

```C
    int main() {
        int a[5];
        int *p
        int *q;
        
        p = a;
        q = p + 3;
        
        return q - p; // expect 3
```

Add additional tests to cover functionality

## Key codegen rule
```C
   p + n => address(p) + n * sizeof(*p)
   p - n => address(p) - n * sizeof(*p)
   p1 - p2 => (address(p1) - address(p2)) / sizeof(*p1)
```

## Out of scope
  `void *` arithmetic