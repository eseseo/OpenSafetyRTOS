/**
 * @file unit_test.h
 * @brief Minimal host-side unit test harness (no external dependencies).
 *
 * Each test executable defines test functions using UT_CHECK and returns
 * ut_summary() from main(): non-zero exit means failure (CTest contract).
 */
#ifndef OSR_UNIT_TEST_H
#define OSR_UNIT_TEST_H

#include <stdio.h>

static int ut_failures = 0;
static int ut_checks = 0;

#define UT_CHECK(cond)                                                     \
    do                                                                     \
    {                                                                      \
        ut_checks++;                                                       \
        if (!(cond))                                                       \
        {                                                                  \
            ut_failures++;                                                 \
            (void)fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, \
                          #cond);                                          \
        }                                                                  \
    } while (0)

#define UT_CHECK_EQ_U32(a, b)                                              \
    do                                                                     \
    {                                                                      \
        ut_checks++;                                                       \
        unsigned long ut_a = (unsigned long)(a);                           \
        unsigned long ut_b = (unsigned long)(b);                           \
        if (ut_a != ut_b)                                                  \
        {                                                                  \
            ut_failures++;                                                 \
            (void)fprintf(stderr,                                          \
                          "FAIL %s:%d: %s == 0x%lX, expected 0x%lX\n",     \
                          __FILE__, __LINE__, #a, ut_a, ut_b);             \
        }                                                                  \
    } while (0)

static int ut_summary(const char *suite)
{
    if (ut_failures == 0)
    {
        (void)printf("%s: %d checks passed\n", suite, ut_checks);
    }
    else
    {
        (void)fprintf(stderr, "%s: %d of %d checks FAILED\n", suite,
                      ut_failures, ut_checks);
    }
    return (ut_failures == 0) ? 0 : 1;
}

#endif /* OSR_UNIT_TEST_H */
