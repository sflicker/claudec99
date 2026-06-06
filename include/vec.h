#ifndef CCOMPILER_VEC_H
#define CCOMPILER_VEC_H

#include <stddef.h>

/* Stage 95-02: generic growable array (vector).
 *
 * Elements are stored as raw bytes; elem_size must match the actual element
 * type used by the caller.  Growth policy: initial capacity 8, doubled on
 * each reallocation.  Allocation failure is a fatal internal error. */
typedef struct {
    void   *data;
    size_t  len;
    size_t  cap;
    size_t  elem_size;
} Vec;

/* Initialize v to an empty vector with the given element size.
 * No memory is allocated until the first push or reserve. */
void vec_init(Vec *v, size_t elem_size);

/* Release the backing store and zero v. */
void vec_free(Vec *v);

/* Ensure at least min_cap elements can be stored without reallocation.
 * The backing store is reallocated to exactly min_cap if the current
 * capacity is smaller.  Returns 1 on success; exits on failure. */
int vec_reserve(Vec *v, size_t min_cap);

/* Append a copy of the element pointed to by elem.
 * Doubles capacity (minimum 8) when the current capacity is exhausted.
 * Returns 1 on success; exits on failure. */
int vec_push(Vec *v, const void *elem);

/* Return a pointer to element at index.
 * Exits with a fatal error if index >= len. */
void *vec_get(Vec *v, size_t index);

/* Remove the last element (decrement len).
 * Exits with a fatal error if the vector is empty. */
void vec_pop(Vec *v);

/* Set len to 0 without freeing the backing store. */
void vec_clear(Vec *v);

#endif /* CCOMPILER_VEC_H */
