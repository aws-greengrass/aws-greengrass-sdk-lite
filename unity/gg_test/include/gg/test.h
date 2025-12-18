#ifndef GG_TEST_UNITY_HELPERS_H
#define GG_TEST_UNITY_HELPERS_H

#ifdef __cplusplus
#include <gg/types.hpp>
#else
#include <gg/buffer.h>
#include <gg/object.h>
#include <stdbool.h>
#endif
#include <sys/types.h>
#include <unity.h>
#include <stddef.h>

// IWYU pragma: begin_keep
#ifdef __cplusplus
#include <gg/error.hpp>
#else
#include <gg/error.h>
#endif
// IWYU pragma: end_keep

typedef void (*GgTestFunction)(void);

typedef struct GgTestListNode {
    GgTestFunction func;
    const char *func_file_name;
    const char *func_name;
    int func_line_num;
    struct GgTestListNode *next;
} GgTestListNode;

extern GgTestListNode *gg_test_list_head;

void gg_test_register(GgTestListNode *entry);

#define GG_TEST_DEFINE(testname) \
    static void test_gg_##testname(void); \
    __attribute__((constructor, no_profile_instrument_function)) static void \
    gg_test_register_##testname(void) { \
        static GgTestListNode entry = { .func = test_gg_##testname, \
                                        .func_file_name = __FILE__, \
                                        .func_name = "test_gg_" #testname, \
                                        .func_line_num = __LINE__, \
                                        .next = NULL }; \
        gg_test_register(&entry); \
    } \
    static void test_gg_##testname(void)

int gg_test_run_suite(void);

#define GG_TEST_FOR_EACH(name) \
    for (GgTestListNode * (name) = gg_test_list_head; (name) != NULL; \
         (name) = (name)->next)

#define GG_TEST_ASSERT_OK(expr) TEST_ASSERT_EQUAL(GG_ERR_OK, (expr))
#define GG_TEST_ASSERT_BAD(expr) TEST_ASSERT_NOT_EQUAL(GG_ERR_OK, (expr))

void gg_test_assert_obj_equal(
    GgObject expected, GgObject actual, const char *message, UNITY_UINT line_no
);

void gg_test_assert_buf_equal(
    GgBuffer expected, GgBuffer actual, const char *message, UNITY_UINT line_no
);

void gg_test_assert_buf_equal_str(
    GgBuffer expected, GgBuffer actual, const char *message, UNITY_UINT line_no
);

void gg_test_assert_kv_equal(
    GgKV expected, GgKV actual, const char *message, UNITY_UINT line_no
);

void gg_test_assert_map_equal(
    GgMap expected, GgMap actual, const char *message, UNITY_UINT line_no
);

void gg_test_assert_list_equal(
    GgList expected, GgList actual, const char *message, UNITY_UINT line_no
);

static inline void gg_test_assert_int64_equal(
    int64_t expected, int64_t actual, const char *message, UNITY_UINT line_no
) {
    UnityAssertEqualNumber(
        (UNITY_INT) expected,
        (UNITY_INT) actual,
        message,
        line_no,
        UNITY_DISPLAY_STYLE_INT64
    );
}

static inline void gg_test_assert_bool_equal(
    bool expected, bool actual, const char *message, UNITY_UINT line_no
) {
    UnityAssertEqualNumber(
        expected, actual, message, line_no, UNITY_DISPLAY_STYLE_INT
    );
}

static inline void gg_test_assert_f64_equal(
    double expected, double actual, const char *message, UNITY_UINT line_no
) {
    UnityAssertDoublesWithin(0.001, expected, actual, message, line_no);
}

#define GG_TEST_ASSERT_OBJ_EQUAL(expected, actual) \
    gg_test_assert_obj_equal((expected), (actual), NULL, __LINE__)

#define GG_TEST_ASSERT_I64_EQUAL(expected, actual) \
    gg_test_assert_i64_equal((expected), (actual), NULL, __LINE__)

#define GG_TEST_ASSERT_BOOL_EQUAL(expected, actual) \
    gg_test_assert_bool_equal((expected), (actual), NULL, __LINE__)

#define GG_TEST_ASSERT_F64_EQUAL(expected, actual) \
    gg_test_assert_f64_equal((expected), (actual), NULL, __LINE__)

#define GG_TEST_ASSERT_BUF_EQUAL(expected, actual) \
    gg_test_assert_buf_equal((expected), (actual), NULL, __LINE__)

#define GG_TEST_ASSERT_BUF_EQUAL_STR(expected, actual) \
    gg_test_assert_buf_equal_str((expected), (actual), NULL, __LINE__)

#define GG_TEST_ASSERT_MAP_EQUAL(expected, actual) \
    gg_test_assert_map_equal((expected), (actual), NULL, __LINE__)

#define GG_TEST_ASSERT_KV_EQUAL(expected, actual) \
    gg_test_assert_kv_equal((expected), (actual), NULL, __LINE__)

#define GG_TEST_ASSERT_LIST_EQUAL(expected, actual) \
    gg_test_assert_list_equal((expected), (actual), NULL, __LINE__)

#ifdef ENABLE_COVERAGE
// NOLINTNEXTLINE(readability-identifier-naming)
extern void __gcov_dump(void);
#undef TEST_PASS
#define TEST_PASS() \
    do { \
        __gcov_dump(); \
        TEST_ABORT(); \
    } while (0)
#endif

#endif
