# ClaudeC99 Stage-20-01  Comma-Separated init-declarator lists

## task
  - Extend the compiler to support comma-separated init-declarator lists
    
## Grammar update
```ebnf
    <declaration> ::= <type-specifier> <init_declarator_list> ";"
    
    <init_declarator_list> ::= <init_declarator> { "," <init_declarator> }

```

## Tests
    ```C
        int main() {
           int a, b;
           a = 3;
           b = 4;
           return a + b;   // expect 7
        }
    
        int main() {
            int a=3, b=4;
            return a * b;   // expect 12
        }
    
        int main() {
            int a = 5, b = a + 2;
            return b;     // expect 7
        }
    
        int main() {
            int *p, q;
            q = 7;
            p = &q;
            return *p;        // expect 7
        }
    
        int main() {
            int a = (1,2), b=3;
            return a + b;    // expect 5
        }
  - ```