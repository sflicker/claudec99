# ClaudeC99 Stage 95-02 - Add dynamic vector foundation

## Task
  - Add a reusable growable array helper
  something like
  ```C
  typedef struct Vec {
    void *data;
    size_t len;
    size_t cap;
    size_t elem_size;
  } Vec;
  ```

  Core operations
  ```C
  void vec_init(Vec *v, size_t elem_size);
  void vec_free(Vec *v);
  int vec_reserve(Vec *v, size_t min_cap);
  int vec_push(Vec *v, const void * elem);
  void * vec_get(Vec *v, size_t index);
  ```

  Growth Policy
  ```text
    Initial capacity: maybe 8, 16, ... or caller provided
    growth: double capacity
    overflow checks: required
    allocation failure: report compiler fatal/internal error cleanly
  ```

  Other useful functions may be added

  This should be added either as a new code module (something like vector.h and vector.c)
  or added to an existing module like util. 

  ## Tests
  The existing testing frameworks probably will not work so 
  make a new one that can test the methods and memory usage
  directory of this new vector module. Ensure adequate test coverage 