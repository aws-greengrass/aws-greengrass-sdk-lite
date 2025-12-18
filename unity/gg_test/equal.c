
#include "gg/test.h"
#include "unity_internals.h"
#include <gg/map.h>
#include <gg/object.h>
#include <gg/object_compare.h>
#include <unity.h>
#include <stdbool.h>

#define GG_TEST_ASSERT_LEN_EQUAL_MESSAGE(expected, actual, message, line_no) \
    UnityAssertEqualNumber( \
        (UNITY_INT) (expected).len, \
        (UNITY_INT) (actual).len, \
        ((message) != NULL ? (message) : "Lengths were not equal"), \
        (UNITY_UINT) (line_no), \
        sizeof(size_t) <= 4 ? UNITY_DISPLAY_STYLE_UINT32 \
                            : UNITY_DISPLAY_STYLE_UINT64 \
    )

void gg_test_assert_obj_equal(
    GgObject expected, GgObject actual, const char *message, UNITY_UINT line_no
) {
    UnityAssertEqualNumber(
        (UNITY_INT) gg_obj_type(expected),
        (UNITY_INT) gg_obj_type(actual),
        message ? message : "Types were not equal",
        line_no,
        UNITY_DISPLAY_STYLE_INT
    );

    if (!gg_obj_eq(expected, actual)) {
        UnityFail(message ? message : "Objects were not equal", line_no);
    };
}

void gg_test_assert_buf_equal(
    GgBuffer expected, GgBuffer actual, const char *message, UNITY_UINT line_no
) {
    GG_TEST_ASSERT_LEN_EQUAL_MESSAGE(expected, actual, message, line_no);

    if (expected.len > 0) {
        UnityAssertEqualIntArray(
            expected.data,
            actual.data,
            (UNITY_UINT32) expected.len,
            message,
            line_no,
            UNITY_DISPLAY_STYLE_UINT8,
            UNITY_ARRAY_TO_ARRAY
        );
    }
}

void gg_test_assert_buf_equal_str(
    GgBuffer expected, GgBuffer actual, const char *message, UNITY_UINT line_no
) {
    GG_TEST_ASSERT_LEN_EQUAL_MESSAGE(expected, actual, message, line_no);

    if (expected.len > 0) {
        UnityAssertEqualStringLen(
            (const char *) expected.data,
            (const char *) actual.data,
            (UNITY_UINT32) expected.len,
            message,
            line_no
        );
    }
}

void gg_test_assert_kv_equal(
    GgKV expected, GgKV actual, const char *message, UNITY_UINT line_no
) {
    gg_test_assert_buf_equal_str(
        gg_kv_key(expected),
        gg_kv_key(actual),
        message ? message : "Keys were not equal",
        line_no
    );

    gg_test_assert_obj_equal(
        *gg_kv_val(&expected),
        *gg_kv_val(&actual),
        message ? message : "Values were not equal",
        line_no
    );
}

void gg_test_assert_map_equal(
    GgMap expected, GgMap actual, const char *message, UNITY_UINT line_no
) {
    GG_TEST_ASSERT_LEN_EQUAL_MESSAGE(expected, actual, message, line_no);

    for (GgKV *expected_kv = expected.pairs, *actual_kv = actual.pairs;
         expected_kv != &expected.pairs[expected.len];
         expected_kv = &expected_kv[1], actual_kv = &actual_kv[1]) {
        gg_test_assert_kv_equal(*expected_kv, *actual_kv, message, line_no);
    }
}

void gg_test_assert_list_equal(
    GgList expected, GgList actual, const char *message, UNITY_UINT line_no
) {
    GG_TEST_ASSERT_LEN_EQUAL_MESSAGE(expected, actual, message, line_no);

    for (GgObject *expected_obj = expected.items, *actual_obj = actual.items;
         expected_obj != &expected.items[expected.len];
         expected_obj = &expected_obj[1], actual_obj = &actual_obj[1]) {
        gg_test_assert_obj_equal(*expected_obj, *actual_obj, message, line_no);
    }
}
