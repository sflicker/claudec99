#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vec.h"

static void vec_fatal(const char *msg) {
    fprintf(stderr, "internal error: %s\n", msg);
    exit(1);
}

void vec_init(Vec *v, size_t elem_size) {
    v->data      = NULL;
    v->len       = 0;
    v->cap       = 0;
    v->elem_size = elem_size;
}

void vec_free(Vec *v) {
    free(v->data);
    v->data      = NULL;
    v->len       = 0;
    v->cap       = 0;
    v->elem_size = 0;
}

int vec_reserve(Vec *v, size_t min_cap) {
    if (v->cap >= min_cap)
        return 1;
    /* Check for size_t overflow in the realloc size computation. */
    if (v->elem_size > 0 && min_cap > (size_t)-1 / v->elem_size)
        vec_fatal("vec_reserve: capacity overflow");
    void *p = realloc(v->data, min_cap * v->elem_size);
    if (!p)
        vec_fatal("vec_reserve: out of memory");
    v->data = p;
    v->cap  = min_cap;
    return 1;
}

int vec_push(Vec *v, const void *elem) {
    if (v->len >= v->cap) {
        size_t new_cap;
        if (v->cap == 0) {
            new_cap = 8;
        } else {
            if (v->cap > (size_t)-1 / 2)
                vec_fatal("vec_push: capacity overflow");
            new_cap = v->cap * 2;
        }
        vec_reserve(v, new_cap);
    }
    memcpy((char *)v->data + v->len * v->elem_size, elem, v->elem_size);
    v->len++;
    return 1;
}

void *vec_get(Vec *v, size_t index) {
    if (index >= v->len)
        vec_fatal("vec_get: index out of bounds");
    return (char *)v->data + index * v->elem_size;
}

void vec_pop(Vec *v) {
    if (v->len == 0)
        vec_fatal("vec_pop: vector is empty");
    v->len--;
}

void vec_clear(Vec *v) {
    v->len = 0;
}
