# ClaudeC99 stage 25-02 assign function designators to function pointers

## Goal
  - Allow function names to be be used as values that can
    initialize or assign compatible function pointers.
  - Reject if incompatible function type with function pointer

## Tests
```C
  int inc(int x) {
      return x + 1;
  }
  
  int main() {
      int (*fp)(int);
      fp = inc;
      return 0;    // expect 0
  } 
  ```
   File-scope support
   ```C
      int inc(int x);
      
      int (*fp)(int) = inc;
      
      int main() {
         return 0;    // expect 0;
      }
   ```

Reject incompatible signatures
Incompatible return type
```C
    int f(int x) { return x; }
    
    int main() {
        long (*fp)(int);
        fp = f;    // INVALID: return type mismatch
    }
```
Incompatible parameters
```C
    int f(int x) { return x; }
    
    int main() {
        int (*fp)(long x);
        fp = f;    // INVALID: parameter type mismatch
    }
```

## Out-of-scope
  - Call through function pointers


