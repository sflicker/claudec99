# ClaudeC99 Stage 37-02 Additional struct tests

## Add additional tests for struct functionality

## Tests
```C
    typedef struct Node Node;
    struct Node {
        int value;
        Node *next;
    };
    
    int main() {
        Node n;
        n.value = 7;
        n.next = &n;
        return n.next->value;  //expect 7
    } 
```

## Invalid Tests
```C
    struct Missing;
    
    int main() {
        struct Missing m;   // INVALID
        return 0;
    } 
```

```C
    struct Missing;
    
    itn main() {
        return sizeof(struct Missing);  // INVALID
        return 0;
    }
```

```C
    struct Node {
        Struct Node next;   // INVALID: recursion by value
        int value;
    };
    
    int main() {
        return 0;
    }
```