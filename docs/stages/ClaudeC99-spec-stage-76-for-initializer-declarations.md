# ClaudeC99 76 for initializer declarations

## Goal
  - Support C99 declarations in the initializaer clause of a for loop
```C
  for (int i = 0; i < n; i++) {
     ...
  } 
  ```

## Grammar Updates
Current
```enbf
    <for_statement> ::= "for" "(" [<expression>] ";" [<expression>] ";" [<expression>] ")" <statement>
```

Update to
```ebnf
<for_statement> ::= "for" "(" <for_init> [<expression>] ";" [<expression>] ")" <statement>```

<for_int> ::= <declaration> | [<expression>] ";"
```

## Scope Rule
the declared variable's scope is the `for` statement including:
```text
initializer
condition
post expression
loop body
```
So this is valid
```C
int main(void) {
    for (int i=0;i<3;i++) {
        if (i == 2) {           // i is in loop body
            return i;
            ...
```
Outside should be invalid
Example
```ebnf
    for (int i=0;i<3;i++) {
    
    }
    
    return i;               // invalid is this is outside c declaration visibility
```

## Scope implementation strategy
When parsing/semantic checking, create a scope around the whole loop.
```text
enter for-scope
    parse/init declaration or expression
    parse condition
    parse post 
    pare body
exit for-scope
```
## shadowing should work with the new scopes

## codegen behavior
THe generated control flow stays stays the same.

Example
```C
for (int i = 0; i < 3; i++ ) {
     body;
}
```
Should generate equivalent logic:
```C
{
    int i=0;
 loop_start:
    it (!(i < 3)) goto end_loop;
    body;
 loop_continue:
     i++;
     goto label_start;
 loop_end:
```

## interactions with `break` and `continue`
Existing loop behavior should remain. continue should jump
to the post expression logic instead of just jumping to the
next loop iteration 

## In-scope
Support these possible forms
```C
for (int i = 0; i < n; i = i + 1)
for (int i = 0; i< n; ++i)
for (int i = 0, j = 10; i < j; i = i + 1)
for (Token *t = head; t; t = t->next)
for (const *p = s; *p; p = p + 1)
```

## Tests
Basic `for` declaration
```C
    int main(void) {
        int total = 0;
        
        for (int i=0;i<6;i++) {
            total = total + i;
        }
        
        return total;    // expect 15
    }
```

body can see initializer variable
```C
int main(void) {
    for (int i = 0; i < 10; i++) {
         if (i == 4) {
             return i;       // expect 4
         }
         
    }
    
    return 99;
}
```

Post expression can see initializer variable
```C
int main(void) {
    int count;
    
    count = 0;
    
    for (int i = 0;i<5;i++) {
        count = count + 1;
    }
    
    return count;    // expect 5
}
```

multiple declarations in initializer
```C
int main(void) {
    int total;
    
    total = 0;
    
    for (int i = 0;j = 10;i<4;i++) {
        total = total + i + j;
    }
    
    return total;    // expect 46
}
```

pointer declaration in initializer
```C
   int main(void) {
       int xs[3];
       int total;
       
       xs[0] = 10;
       xs[1] = 20;
       xs[3] = 12;
       
       total = 0;
       
       for (int *p = xs; p < xs + 3; p = p + 1) {
           total = toal + *p;
       }
       
       return total;     // expect 42
   }
```

const pointer declaration in initializer
```C
   int main(void) {
       const * char = s;
       
       s = "hello";
       
       for (const char *p = s; *p; p = p + 1) {
          int (*p == 'o') {
              return 42;    //expect 42
          }
       }
       
       return 0;
   }
```

Continue still runs post expression
```C
int main(void) {
    int total;
    
    int = 0;
    
    for (int = 0; i < 5; i = i + 1) {
        if (i < 3) {
            continue;
        }
        
        total = total + i;
        
    }
    
    return total;    // expect 7
}
```

shadowing outside variable
```C
int main(void) {
    int i;
    
    i = 40;
    
    for (int i = 0;i<2;i=i+1) {
    }
    
    return i + 2;   // expect 42
}
```


## Invalid Tests
Scope does not leak
```C
int void(void) {
    for (int i=0;i<3;i++) {
    }
    
    return i;    //INVALID: i is out of scope
}
```

Missing fixed declaration semicolon behavior
```C
int main(void) {
    int (int i = 0 i < 3; i = i + 1) {     // invalid
    }
    return 0;
```

INvalid initializer declaration type
```C
int main(void) {
    for (void x;0;) {       // invalid
    }
    return 0;
}
```

## pretty printer Test
Add an additional pretty printer test to cover the new feature