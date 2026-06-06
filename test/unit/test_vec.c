/*
 * Stage 95-02: unit tests for Vec (generic growable array).
 *
 * Compile independently of the compiler binary:
 *   gcc -std=c99 -Wall -I../../include ../../src/vec.c test_vec.c -o test_vec
 *   ./test_vec
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vec.h"

/* ---- minimal test framework ---- */

static int g_pass = 0;
static int g_fail = 0;

#define ASSERT(cond) do { \
    if (cond) { \
        printf("  PASS  %s\n", #cond); \
        g_pass++; \
    } else { \
        printf("  FAIL  %s  (line %d)\n", #cond, __LINE__); \
        g_fail++; \
    } \
} while (0)

#define ASSERT_EQ_SIZE(a, b) ASSERT((size_t)(a) == (size_t)(b))
#define ASSERT_EQ_INT(a, b)  ASSERT((int)(a) == (int)(b))

static void section(const char *name) {
    printf("\n[%s]\n", name);
}

/* ---- test helpers ---- */

typedef struct { int x; int y; } Point;

/* ---- tests ---- */

static void test_init(void) {
    section("vec_init");
    Vec v;
    vec_init(&v, sizeof(int));
    ASSERT(v.data == NULL);
    ASSERT_EQ_SIZE(v.len, 0);
    ASSERT_EQ_SIZE(v.cap, 0);
    ASSERT_EQ_SIZE(v.elem_size, sizeof(int));
    vec_free(&v);
}

static void test_free(void) {
    section("vec_free");
    Vec v;
    vec_init(&v, sizeof(int));
    int val = 42;
    vec_push(&v, &val);
    ASSERT(v.data != NULL);
    ASSERT_EQ_SIZE(v.len, 1);
    vec_free(&v);
    ASSERT(v.data == NULL);
    ASSERT_EQ_SIZE(v.len, 0);
    ASSERT_EQ_SIZE(v.cap, 0);
    ASSERT_EQ_SIZE(v.elem_size, 0);
}

static void test_push_and_get_int(void) {
    section("vec_push / vec_get (int)");
    Vec v;
    vec_init(&v, sizeof(int));

    int val;
    for (int i = 0; i < 10; i++) {
        val = i * i;
        ASSERT_EQ_INT(vec_push(&v, &val), 1);
    }
    ASSERT_EQ_SIZE(v.len, 10);

    for (int i = 0; i < 10; i++) {
        int *p = (int *)vec_get(&v, (size_t)i);
        ASSERT(p != NULL);
        ASSERT_EQ_INT(*p, i * i);
    }
    vec_free(&v);
}

static void test_push_and_get_struct(void) {
    section("vec_push / vec_get (struct)");
    Vec v;
    vec_init(&v, sizeof(Point));

    for (int i = 0; i < 5; i++) {
        Point p = { i, i * 2 };
        vec_push(&v, &p);
    }
    ASSERT_EQ_SIZE(v.len, 5);

    for (int i = 0; i < 5; i++) {
        Point *p = (Point *)vec_get(&v, (size_t)i);
        ASSERT_EQ_INT(p->x, i);
        ASSERT_EQ_INT(p->y, i * 2);
    }
    vec_free(&v);
}

static void test_growth(void) {
    section("growth (capacity doubles)");
    Vec v;
    vec_init(&v, sizeof(int));

    /* First push should allocate with cap=8. */
    int val = 1;
    vec_push(&v, &val);
    ASSERT_EQ_SIZE(v.cap, 8);
    ASSERT_EQ_SIZE(v.len, 1);

    /* Fill to capacity=8. */
    for (int i = 1; i < 8; i++) {
        vec_push(&v, &val);
    }
    ASSERT_EQ_SIZE(v.len, 8);
    ASSERT_EQ_SIZE(v.cap, 8);

    /* One more push should double cap to 16. */
    vec_push(&v, &val);
    ASSERT_EQ_SIZE(v.len, 9);
    ASSERT_EQ_SIZE(v.cap, 16);

    /* Fill to 16. */
    for (int i = 9; i < 16; i++) {
        vec_push(&v, &val);
    }
    ASSERT_EQ_SIZE(v.len, 16);
    ASSERT_EQ_SIZE(v.cap, 16);

    /* One more push should double cap to 32. */
    vec_push(&v, &val);
    ASSERT_EQ_SIZE(v.cap, 32);

    vec_free(&v);
}

static void test_reserve(void) {
    section("vec_reserve");
    Vec v;
    vec_init(&v, sizeof(int));

    /* Reserve with current cap=0. */
    ASSERT_EQ_INT(vec_reserve(&v, 20), 1);
    ASSERT(v.cap >= 20);
    ASSERT_EQ_SIZE(v.len, 0);

    /* Reserve less than current cap: no-op. */
    size_t old_cap = v.cap;
    ASSERT_EQ_INT(vec_reserve(&v, 5), 1);
    ASSERT_EQ_SIZE(v.cap, old_cap);

    /* Push elements; verify they land in the reserved space. */
    for (int i = 0; i < 20; i++) {
        int val = i;
        vec_push(&v, &val);
    }
    ASSERT_EQ_SIZE(v.len, 20);

    /* Verify values. */
    for (int i = 0; i < 20; i++) {
        int *p = (int *)vec_get(&v, (size_t)i);
        ASSERT_EQ_INT(*p, i);
    }
    vec_free(&v);
}

static void test_pop(void) {
    section("vec_pop");
    Vec v;
    vec_init(&v, sizeof(int));

    for (int i = 0; i < 5; i++) {
        vec_push(&v, &i);
    }
    ASSERT_EQ_SIZE(v.len, 5);

    vec_pop(&v);
    ASSERT_EQ_SIZE(v.len, 4);

    /* Top element should now be 3. */
    int *top = (int *)vec_get(&v, 3);
    ASSERT_EQ_INT(*top, 3);

    vec_pop(&v);
    vec_pop(&v);
    vec_pop(&v);
    vec_pop(&v);
    ASSERT_EQ_SIZE(v.len, 0);

    vec_free(&v);
}

static void test_clear(void) {
    section("vec_clear");
    Vec v;
    vec_init(&v, sizeof(int));

    for (int i = 0; i < 10; i++) {
        vec_push(&v, &i);
    }
    ASSERT_EQ_SIZE(v.len, 10);
    size_t old_cap = v.cap;

    vec_clear(&v);
    ASSERT_EQ_SIZE(v.len, 0);
    /* Backing store is retained. */
    ASSERT_EQ_SIZE(v.cap, old_cap);
    ASSERT(v.data != NULL);

    /* Can reuse the vector after clear. */
    int val = 99;
    vec_push(&v, &val);
    ASSERT_EQ_SIZE(v.len, 1);
    ASSERT_EQ_INT(*(int *)vec_get(&v, 0), 99);

    vec_free(&v);
}

static void test_stress(void) {
    section("stress (1000 int pushes)");
    Vec v;
    vec_init(&v, sizeof(int));

    for (int i = 0; i < 1000; i++) {
        vec_push(&v, &i);
    }
    ASSERT_EQ_SIZE(v.len, 1000);

    int ok = 1;
    for (int i = 0; i < 1000; i++) {
        int *p = (int *)vec_get(&v, (size_t)i);
        if (*p != i) { ok = 0; break; }
    }
    ASSERT(ok);

    vec_free(&v);
}

static void test_multiple_types(void) {
    section("multiple independent vecs");
    Vec vi, vc, vp;
    vec_init(&vi, sizeof(int));
    vec_init(&vc, sizeof(char));
    vec_init(&vp, sizeof(Point));

    for (int i = 0; i < 5; i++) {
        int  iv = i;          vec_push(&vi, &iv);
        char cv = (char)('a' + i); vec_push(&vc, &cv);
        Point pv = { i, -i };  vec_push(&vp, &pv);
    }

    ASSERT_EQ_SIZE(vi.len, 5);
    ASSERT_EQ_SIZE(vc.len, 5);
    ASSERT_EQ_SIZE(vp.len, 5);

    ASSERT_EQ_INT(*(int *)vec_get(&vi, 3), 3);
    ASSERT_EQ_INT(*(char *)vec_get(&vc, 2), 'c');
    Point *pp = (Point *)vec_get(&vp, 4);
    ASSERT_EQ_INT(pp->x, 4);
    ASSERT_EQ_INT(pp->y, -4);

    vec_free(&vi);
    vec_free(&vc);
    vec_free(&vp);
}

/* ---- main ---- */

int main(void) {
    printf("Vec unit tests\n");
    printf("==============\n");

    test_init();
    test_free();
    test_push_and_get_int();
    test_push_and_get_struct();
    test_growth();
    test_reserve();
    test_pop();
    test_clear();
    test_stress();
    test_multiple_types();

    printf("\nResults: %d passed, %d failed, %d total\n",
           g_pass, g_fail, g_pass + g_fail);
    return g_fail > 0 ? 1 : 0;
}
