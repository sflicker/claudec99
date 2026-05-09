# ClaudeC99 Stage 28-02 PointerTypedef

## Task
  - Extend typedef to support pointers

## Tests
   ```C
        typedef int *IntPtr;
        
        int main() {
           int x = 7;
           IntPtr p = &x;
           return *p;          // expect 7
        }
   ```

   ```C
       typedef char * String;
       
       int main() {
           char c = 65;
           String p= &c;
           return *p;          // expect 65
       }
   ```

   ```C
      typedef int I;
      typedef I *IPtr;

      int main() {
          int x = 9;
          IPtr p = &x;
          return *p;         // expect 9
      }
   ```

   ```C
       typedef int *P;
       P a,b;
       
       int main() {
           int x = 9;
           a = &9;
           b = a;
           return *b;        // expect 9
       }
   ```
