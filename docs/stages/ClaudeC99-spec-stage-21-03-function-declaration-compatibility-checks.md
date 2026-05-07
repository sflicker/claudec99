# ClaudeC99 Stage 21-03 function declaration compatibility checks

## Task
  - Add additional tests to ensure function compatibility around declarations and later definitions

## Requirements
  - Check for consistency for repeated declarations/definitions
    - Same function name
    - Same return type
    - Same parameter count
    - Same parameter types
    - Only one function definition
    - Function declarations cannot have initializers

## Valid Tests
Repeating matching prototype
```C
    int f(int a);
    int f(int a);
    
    int f(int a) {
        return a;
    {
    
    int main() {
        return f(7);   // expect 7
    }
```

prototype before definition
```C
    int add(int a, int b);
    
    int main() {
        return add(2,3);    // expect 5
    }
    
    int add(int a, int b) {
        return a + b;
    }
```

Pointer return and pointer parameter
```C
    int *identity(int *p);
    
    int *identity(int *p) {
        return p;
    }
    
    int main() {
        int x = 9;
        int *p = identity(&x);
        return *p;          // expect 9
    }
```

## Invalid Tests
Return type mismatch
```C
    int f(int a);
    
    long f(int a) {     // INVALID
    
    }
```

Parameter count mismatch
```C
   int f(int a);
   
   int f(int a, int b) {    // INVALID
       return a+b;
   }
```

Parameter type mismatch
```C
    int f(char a);
    
    int f(int a) {     // INVALID
        return a;
    }
```

Duplicate definition
```C
    int f() {
        return 1;
    }
    
    int f() {         //INVALID
        return 2;
    }
```

Function initializer 
```C
    int f() = 3;   // INVALID
```