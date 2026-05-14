# ClaudeC99 - Stage 42 - array of pointers

## Tasks
  - Add additional tests to verify array of pointers
  - Rules that should be enforced
    - pointer = compatible pointer      allowed
    - pointer = void *                  allowed
    - void * = object pointer           allowed
    - pointer = 0                       allowed as null pointer
    - pointer = nonzero integer         reject
    - 
Basic `int *` array
## Tests
```C
    int main() {
        int a;
        int b;
        int *items[2];
        
        a = 10;
        b = 20;
        
        items[0] = &a;
        items[1] = &b;
        
        return *items[0] + *items[1]; // expect 30
    }
```

```C
    int main() {
        char *names[2];
        
        names[0] = "ab";
        names[1] = "cd";
        
        return names[0][0] + names[1][0]; // expect 196: 'a' + 'c'
    }
```

Mutate through pointer array
```C
    int a;
    int b;
    int *items[2];
    
    a = 1;
    b = 2;
    
    items[0] = &a;
    items[1] = &b;
    
    *items[0] = 10;
    *items[1] = 20;
    
    return a + b;  //expect 30
}
```

Pointer array element reassignment
```C
    int main() {
        int a;
        int b;
        int *items[2];
        
        a = 7;
        b = 9;
        
        items[0] = &a;
        items[1] = items[0];
        
        return *items[1];  //expect 7
    }
```

Reassign pointer array element
```C
    int main() {
        int a;
        int b;
        int *items[1];
        
        a = 11;
        b = 22;
        
        items[0] = &a;
        items[0] = &b;
        
        return *items[0];   // expect 22
```

`char *` array with string literals
```C
    int main() {
        char *names[2];
        
        names[0] = "ab";
        names[1] = "cd";
        
        return names[0][1] + names[1][1];  // expect 198: 'b' + 'c'
```

pointer arithmetic through array element
```C
    int main() {
        char *names[1];
        char *p;
        
        names[0] = "abc";
        p = names[0] + 2;
        
        return *p;  // expect 99: 'c'
```

array of struct pointers
```C
    struct Point {
        int x;
        int y;
    };
    
    int main() {
        struct Point a;
        struct Point b;
        struct Point *items[2];
        
        a.x = 1;
        a.y = 2;
        b.x = 10;
        b.y = 30;
        
        items[0] = &a;
        items[1] = &b;
        
        return items[0]->x + items[0]->y + items[1]->x + items[1]->y; // expect 33
    }
```

mutate struct through pointer array
```C
    struct Counter {
        int value;
    }
    
    int main() {
        struct Counter a;
        struct Counter b;
        struct Counter *items[2];
        
        a.value = 1;
        b.value = 2;
        
        items[0] = &a;
        items[1] = &b;
        
        items[0]->value = 10;
        items[1]->value = 20;
        
        return a.value + b.value;  // expect 30
```

typedef pointer array
```C
typedef int *IntPtr;

int main() {
    int a;
    int b;
    IntPtr Items[2];
    
    a = 5;
    b = 6;
    
    items[0] = &a;
    items[1] = &b;
    
    return *items[0] + *items[1];  // expect 11
```

typedef struct pointer array
```C
    typedef struct Node Node;
    
    struct Node {
        int value;
        Node *next;
    };
    
    int main() {
        Node a;
        Node b;
        Node *items[2];
        
        a.value = 10;
        b.value = 20;
        
        items[0] = &a;
        items[1] = &b;
        
        return items[0]->value + items[1]->value;  // expect 30
    }
```

Pointer-to-pointer from array decay
```C
    int sum_first_two(int **items) {
        return *items[0] + *items[1];
    }
    
    int main() {
        int a;
        int b;
        int *items[2];
        
        a = 13;
        b = 17;
        
        items[0] = &a;
        items[1] = &b;
        
        return sum_first_two(items);  // expect 30
    }
```

arrays of `void *`

```C
    int main() {
        int x;
        int *p;
        
        void *items[2];
        
        x = 42;
        
        items[0] = &x;    // int * -> void * is OK
        p = items[0];     // void * -> int * is OK
        
        return *p;   // expect 42
    }
```

Store different object pointer types
```C
struct Point {
    int x;
    int y;
};

int main() {
    int x;
    struct Point p;
    int *ip;
    struct Point *pp;
    void *items[2];

    x = 10;
    p.x = 20;
    p.y = 30;

    items[0] = &x;
    items[1] = &p;

    ip = items[0];
    pp = items[1];

    return *ip + pp->x + pp->y;   // expect 60
}
```

null pointer in void * array
```C
int main() {
    void *items[2];

    items[0] = 0;
    items[1] = 0;

    if (items[0] == items[1])
        return 42;

    return 1;
}
```

## Invalid tests

assign non-pointer to pointer array element
```C
    int main() {
        int *items[2];
        
        items[0] = 123;     // REJECT: assigning nonzero integer to pointer
        return 0;
    }
```

Assign wrong pointer type
```C
    int main() {
        int x;
        char *items[1];
        
        items[0] = &x;    // REJECT: int * assigned to char *
    }        
```

Cannot dereference `void *` element directly
```C
    int main() {
        void *items[1];
        
        x = 42;
        items[0] = &x;
        
        return *items[0];   // REJECT: cannot dereference void *
    }
```

Cannot index through `void *`
```C
    int main () {
        char *s;
        void *items[1];
        
        s = "abc";
        items[0] = s;
        
        return items[0][0];   // REJECT: items[0] is void *
    }
```