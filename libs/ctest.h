/* 
 * minimalistic C test framework inspired by:
 * http://www.jera.com/techinfo/jtns/jtn002.html
 * 
 * documentation on github:
 * https://github.com/mikenoethiger/ctest
 */

#include <stdio.h>

#define VERSION "1.0"

#define STR(x) #x
#define assrt_m(assert, msg) if (!ctest_assrt(assert, STR(assert), __func__, __LINE__, msg)) return 0;
#define assrt(assert) if (!ctest_assrt(assert, STR(assert), __func__, __LINE__, NULL)) return 0;

#define ctest_run(test) do { int result = test(); tests_run++; if (result) tests_passed++; else tests_failed++; } while (0)

int tests_run = 0;
int tests_failed = 0;
int tests_passed = 0;
int tests_assertions = 0;

int ctest_assrt(int assert_result, char *assert_expr, const char *func, int line, char *msg) {
    if (!assert_result) {
        printf("%s:%d failed:\n", func, line);
        if (msg) printf("\t%s: %s\n", msg, assert_expr);
        else printf("\t%s\n", assert_expr);
    }
    tests_assertions++;
    return assert_result;
}

void ctest_print_result() {
    printf("\n");
    if (tests_run == tests_passed) {
        printf("ALL TESTS PASSED\n");
    }
    printf("%d assertions\n", tests_assertions);
    printf("%d tests, %d passed, %d failed\n", tests_run, tests_passed, tests_failed);
}

void ctest_print_version() {
    printf("v%s\n", VERSION);
}
