#ifndef GG_TEST_UNITY_HELPERS_H
#define GG_TEST_UNITY_HELPERS_H

#include <sys/types.h>
#include <unity.h>
#include <stddef.h>

typedef void (*GgTestFunction)(void);

typedef struct GgTestListNode {
    GgTestFunction func;
    const char *func_file_name;
    const char *func_name;
    int func_line_num;
    struct GgTestListNode *next;
} GgTestListNode;

extern int server_handle;
extern GgTestListNode *gg_test_list_head;

void gg_test_register(GgTestListNode *entry);

#define GG_TEST_DEFINE(testname) \
    static void test_gg_##testname(void); \
    __attribute__((constructor)) static void gg_test_register_##testname( \
        void \
    ) { \
        static GgTestListNode entry = { .func = test_gg_##testname, \
                                        .func_file_name = __FILE__, \
                                        .func_name = "test_gg_" #testname, \
                                        .func_line_num = __LINE__, \
                                        .next = NULL }; \
        gg_test_register(&entry); \
    } \
    static void test_gg_##testname(void)

#define GG_TEST_FOR_EACH(name) \
    for (GgTestListNode * (name) = gg_test_list_head; (name) != NULL; \
         (name) = (name)->next)

#define GG_TEST_ASSERT_OK(expr) TEST_ASSERT_EQUAL(GG_ERR_OK, (expr))
#define GG_TEST_ASSERT_BAD(expr) TEST_ASSERT_NOT_EQUAL(GG_ERR_OK, (expr))

#define GG_TEST_ASSERT_BUF_EQUAL(expected, actual) \
    do { \
        TEST_ASSERT_EQUAL_size_t_MESSAGE( \
            (expected).len, (actual).len, "Size mismatch" \
        ); \
        TEST_ASSERT_EQUAL_CHAR_ARRAY( \
            (char *) (expected).data, (char *) (actual).data, (expected).len \
        ); \
    } while (0)

int gg_test_run_suite(void);

#endif
