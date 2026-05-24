#ifndef CANMOD_TEST_RUNNER_H
#define CANMOD_TEST_RUNNER_H

/**
 * Minimal test runner — no external dependencies.
 *
 * Usage in each test file:
 *
 *   #include "test_runner.h"
 *
 *   int main(void) {
 *       TEST_ASSERT(1 + 1 == 2);
 *       TEST_ASSERT_EQ(car_get_speed(), 5000u);
 *       return test_runner_result();
 *   }
 */

#include <stdio.h>
#include <string.h>

static int g_test_failures = 0;
static int g_test_count    = 0;

/** Assert a condition. Prints FAIL with file:line on failure. */
#define TEST_ASSERT(cond) do {                                              \
    g_test_count++;                                                         \
    if (!(cond)) {                                                          \
        fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
        g_test_failures++;                                                  \
    }                                                                       \
} while (0)

/** Assert two values are equal (printed as integers on failure). */
#define TEST_ASSERT_EQ(a, b) do {                                           \
    g_test_count++;                                                         \
    if ((a) != (b)) {                                                       \
        fprintf(stderr, "FAIL: %s:%d: %s == %s  (%ld != %ld)\n",          \
                __FILE__, __LINE__, #a, #b,                                 \
                (long)(a), (long)(b));                                      \
        g_test_failures++;                                                  \
    }                                                                       \
} while (0)

/** Assert a byte buffer matches an expected sequence. */
#define TEST_ASSERT_BYTES(buf, expected, len) do {                          \
    g_test_count++;                                                         \
    int _ok = (memcmp((buf), (expected), (len)) == 0);                     \
    if (!_ok) {                                                             \
        fprintf(stderr, "FAIL: %s:%d: byte mismatch in %s\n",             \
                __FILE__, __LINE__, #buf);                                  \
        fprintf(stderr, "  expected:");                                     \
        for (int _i = 0; _i < (int)(len); _i++)                           \
            fprintf(stderr, " %02X", ((unsigned char*)(expected))[_i]);    \
        fprintf(stderr, "\n  got:     ");                                   \
        for (int _i = 0; _i < (int)(len); _i++)                           \
            fprintf(stderr, " %02X", ((unsigned char*)(buf))[_i]);         \
        fprintf(stderr, "\n");                                              \
        g_test_failures++;                                                  \
    }                                                                       \
} while (0)

/** Assert a string contains a substring. */
#define TEST_ASSERT_CONTAINS(haystack, needle) do {                         \
    g_test_count++;                                                         \
    if (strstr((haystack), (needle)) == NULL) {                             \
        fprintf(stderr, "FAIL: %s:%d: \"%s\" not found in output\n",      \
                __FILE__, __LINE__, (needle));                               \
        fprintf(stderr, "  output: %s\n", (haystack));                     \
        g_test_failures++;                                                  \
    }                                                                       \
} while (0)

/** Print summary and return exit code (0 = all pass, 1 = any failure). */
static inline int test_runner_result(void) {
    if (g_test_failures == 0) {
        printf("PASS (%d assertions)\n", g_test_count);
        return 0;
    }
    printf("FAIL (%d/%d assertions failed)\n", g_test_failures, g_test_count);
    return 1;
}

#endif /* CANMOD_TEST_RUNNER_H */
