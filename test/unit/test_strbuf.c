/*
 * Stage 95-03: unit tests for StrBuf (dynamic character/string buffer).
 *
 * Compile independently of the compiler binary:
 *   gcc -std=c99 -Wall -I../../include ../../src/strbuf.c test_strbuf.c -o test_strbuf
 *   ./test_strbuf
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strbuf.h"

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
#define ASSERT_STR_EQ(a, b)  ASSERT(strcmp((a), (b)) == 0)

static void section(const char *name) {
    printf("\n[%s]\n", name);
}

/* ---- tests ---- */

static void test_init(void) {
    section("strbuf_init");
    StrBuf b;
    strbuf_init(&b);
    ASSERT(b.data == NULL);
    ASSERT_EQ_SIZE(b.len, 0);
    ASSERT_EQ_SIZE(b.cap, 0);
    strbuf_free(&b);
}

static void test_free(void) {
    section("strbuf_free");
    StrBuf b;
    strbuf_init(&b);
    strbuf_append_char(&b, 'x');
    ASSERT(b.data != NULL);
    ASSERT_EQ_SIZE(b.len, 1);
    strbuf_free(&b);
    ASSERT(b.data == NULL);
    ASSERT_EQ_SIZE(b.len, 0);
    ASSERT_EQ_SIZE(b.cap, 0);
}

static void test_append_char(void) {
    section("strbuf_append_char");
    StrBuf b;
    strbuf_init(&b);

    ASSERT_EQ_INT(strbuf_append_char(&b, 'h'), 1);
    ASSERT_EQ_INT(strbuf_append_char(&b, 'i'), 1);
    ASSERT_EQ_SIZE(b.len, 2);
    ASSERT_EQ_INT(b.data[0], 'h');
    ASSERT_EQ_INT(b.data[1], 'i');

    strbuf_free(&b);
}

static void test_append_str(void) {
    section("strbuf_append_str");
    StrBuf b;
    strbuf_init(&b);

    ASSERT_EQ_INT(strbuf_append_str(&b, "hello"), 1);
    ASSERT_EQ_SIZE(b.len, 5);

    ASSERT_EQ_INT(strbuf_append_str(&b, " world"), 1);
    ASSERT_EQ_SIZE(b.len, 11);

    strbuf_null_terminate(&b);
    ASSERT_STR_EQ(b.data, "hello world");

    strbuf_free(&b);
}

static void test_append_str_empty(void) {
    section("strbuf_append_str (empty string)");
    StrBuf b;
    strbuf_init(&b);

    strbuf_append_str(&b, "abc");
    ASSERT_EQ_INT(strbuf_append_str(&b, ""), 1);
    ASSERT_EQ_SIZE(b.len, 3);

    strbuf_free(&b);
}

static void test_append_n(void) {
    section("strbuf_append_n");
    StrBuf b;
    strbuf_init(&b);

    ASSERT_EQ_INT(strbuf_append_n(&b, "hello world", 5), 1);
    ASSERT_EQ_SIZE(b.len, 5);

    strbuf_null_terminate(&b);
    ASSERT_STR_EQ(b.data, "hello");

    strbuf_free(&b);
}

static void test_append_n_zero(void) {
    section("strbuf_append_n (n=0)");
    StrBuf b;
    strbuf_init(&b);

    ASSERT_EQ_INT(strbuf_append_n(&b, "abc", 0), 1);
    ASSERT_EQ_SIZE(b.len, 0);

    strbuf_free(&b);
}

static void test_null_terminate(void) {
    section("strbuf_null_terminate");
    StrBuf b;
    strbuf_init(&b);

    strbuf_append_str(&b, "foo");
    ASSERT_EQ_SIZE(b.len, 3);

    ASSERT_EQ_INT(strbuf_null_terminate(&b), 1);
    /* len does not change */
    ASSERT_EQ_SIZE(b.len, 3);
    /* data is now a valid C string */
    ASSERT_EQ_INT(b.data[3], '\0');
    ASSERT_STR_EQ(b.data, "foo");

    strbuf_free(&b);
}

static void test_null_terminate_empty(void) {
    section("strbuf_null_terminate (empty buffer)");
    StrBuf b;
    strbuf_init(&b);

    ASSERT_EQ_INT(strbuf_null_terminate(&b), 1);
    ASSERT_EQ_SIZE(b.len, 0);
    ASSERT_EQ_INT(b.data[0], '\0');

    strbuf_free(&b);
}

static void test_reserve(void) {
    section("strbuf_reserve");
    StrBuf b;
    strbuf_init(&b);

    ASSERT_EQ_INT(strbuf_reserve(&b, 32), 1);
    ASSERT(b.cap >= 32);
    ASSERT_EQ_SIZE(b.len, 0);

    /* Reserve less than current cap: no-op. */
    size_t old_cap = b.cap;
    ASSERT_EQ_INT(strbuf_reserve(&b, 4), 1);
    ASSERT_EQ_SIZE(b.cap, old_cap);

    strbuf_free(&b);
}

static void test_growth(void) {
    section("growth (capacity doubles)");
    StrBuf b;
    strbuf_init(&b);

    /* First char should allocate with cap=8. */
    strbuf_append_char(&b, 'a');
    ASSERT_EQ_SIZE(b.cap, 8);
    ASSERT_EQ_SIZE(b.len, 1);

    /* Fill to capacity=8. */
    for (int i = 1; i < 8; i++) {
        strbuf_append_char(&b, 'b');
    }
    ASSERT_EQ_SIZE(b.len, 8);
    ASSERT_EQ_SIZE(b.cap, 8);

    /* One more should double cap to 16. */
    strbuf_append_char(&b, 'c');
    ASSERT_EQ_SIZE(b.len, 9);
    ASSERT_EQ_SIZE(b.cap, 16);

    strbuf_free(&b);
}

static void test_append_str_growth(void) {
    section("strbuf_append_str growth across realloc");
    StrBuf b;
    strbuf_init(&b);

    /* Append in small pieces to trigger multiple reallocations. */
    for (int i = 0; i < 20; i++) {
        strbuf_append_str(&b, "ab");
    }
    ASSERT_EQ_SIZE(b.len, 40);

    strbuf_null_terminate(&b);
    /* Spot-check a few positions. */
    ASSERT_EQ_INT(b.data[0], 'a');
    ASSERT_EQ_INT(b.data[1], 'b');
    ASSERT_EQ_INT(b.data[38], 'a');
    ASSERT_EQ_INT(b.data[39], 'b');
    ASSERT_EQ_INT(b.data[40], '\0');

    strbuf_free(&b);
}

static void test_stress(void) {
    section("stress (1000 char appends)");
    StrBuf b;
    strbuf_init(&b);

    for (int i = 0; i < 1000; i++) {
        strbuf_append_char(&b, (char)('a' + i % 26));
    }
    ASSERT_EQ_SIZE(b.len, 1000);

    /* Verify a few values. */
    ASSERT_EQ_INT(b.data[0],   'a');
    ASSERT_EQ_INT(b.data[25],  'z');
    ASSERT_EQ_INT(b.data[26],  'a');
    ASSERT_EQ_INT(b.data[999], (char)('a' + 999 % 26));

    strbuf_free(&b);
}

static void test_mixed_appends(void) {
    section("mixed append_char / append_str / append_n");
    StrBuf b;
    strbuf_init(&b);

    strbuf_append_char(&b, '[');
    strbuf_append_str(&b, "foo");
    strbuf_append_n(&b, ",bar", 4);
    strbuf_append_char(&b, ']');
    ASSERT_EQ_SIZE(b.len, 9);

    strbuf_null_terminate(&b);
    ASSERT_STR_EQ(b.data, "[foo,bar]");

    strbuf_free(&b);
}

static void test_reuse_after_free(void) {
    section("reuse after strbuf_free");
    StrBuf b;
    strbuf_init(&b);

    strbuf_append_str(&b, "first");
    strbuf_free(&b);

    strbuf_init(&b);
    strbuf_append_str(&b, "second");
    strbuf_null_terminate(&b);
    ASSERT_STR_EQ(b.data, "second");
    ASSERT_EQ_SIZE(b.len, 6);

    strbuf_free(&b);
}

/* ---- main ---- */

int main(void) {
    printf("StrBuf unit tests\n");
    printf("=================\n");

    test_init();
    test_free();
    test_append_char();
    test_append_str();
    test_append_str_empty();
    test_append_n();
    test_append_n_zero();
    test_null_terminate();
    test_null_terminate_empty();
    test_reserve();
    test_growth();
    test_append_str_growth();
    test_stress();
    test_mixed_appends();
    test_reuse_after_free();

    printf("\nResults: %d passed, %d failed, %d total\n",
           g_pass, g_fail, g_pass + g_fail);
    return g_fail > 0 ? 1 : 0;
}
