#include "gg/map.h"
#include <gg/log.h>

static void swap_pairs(GgKV *lhs, GgKV *rhs) {
    GgKV temp = *lhs;
    *lhs = *rhs;
    *rhs = temp;
}

static bool is_key_less(GgKV lhs, GgKV rhs) {
    GgBuffer lhs_buf = gg_kv_key(lhs);
    GgBuffer rhs_buf = gg_kv_key(rhs);

    size_t len = lhs_buf.len > rhs_buf.len ? rhs_buf.len : lhs_buf.len;
    for (size_t i = 0; i != len; ++i) {
        if (lhs_buf.data[i] < rhs_buf.data[i]) {
            return true;
        }
        if (rhs_buf.data[i] < lhs_buf.data[i]) {
            return false;
        }
    }

    return lhs_buf.len < rhs_buf.len;
}

static void prune_duplicates(GgMap *val) {
    // The last occurence of a key must be a duplicate
    // if the first occurence of that key appears elsewhere.
    for (size_t i = val->len; i > 0; --i) {
        GgKV *pair = &val->pairs[i - 1];
        GgObject *last = gg_kv_val(pair);
        GgObject *first = NULL;
        (void) gg_map_get(*val, gg_kv_key(*pair), &first);
        if (first == last) {
            continue;
        }

        GG_LOGW(
            "Duplicate key \"%.*s\" found in map",
            (int) gg_kv_key(*pair).len,
            gg_kv_key(*pair).data
        );

        if (i != val->len) {
            *pair = val->pairs[val->len - 1];
        }
        gg_kv_set_key(&val->pairs[val->len - 1], GG_STR("<pruned>"));
        *gg_kv_val(&val->pairs[val->len - 1]) = GG_OBJ_NULL;
        val->len--;
    }
}

static void sort_keys(GgMap *val) {
    // insertion sort
    for (size_t i = 0; i < val->len; ++i) {
        size_t j = i;
        while ((j > 0) && (is_key_less(val->pairs[j], val->pairs[j - 1]))) {
            swap_pairs(&val->pairs[j], &val->pairs[j - 1]);
            --j;
        }
    }
}

void gg_map_canonicalize_shallow(GgMap *map) {
    prune_duplicates(map);
    sort_keys(map);
}

bool gg_map_is_canonical(GgMap map) {
    for (size_t i = 1; i < map.len; ++i) {
        GgKV lesser = map.pairs[i - 1];
        GgKV greater = map.pairs[i];
        // All keys must be strictly-increasing (no duplicates, sorted)
        if (!is_key_less(lesser, greater)) {
            return false;
        }
    }
    return true;
}

#ifdef GG_SDK_TESTING

#include <gg/test.h>
#include <unity.h>

static bool test_buf_compare(GgBuffer lhs, GgBuffer rhs) {
    return is_key_less(gg_kv(lhs, GG_OBJ_NULL), gg_kv(rhs, GG_OBJ_NULL));
}

GG_TEST_DEFINE(kv_key_comparisons) {
    TEST_ASSERT(test_buf_compare(GG_STR("a"), GG_STR("b")));
    TEST_ASSERT(test_buf_compare(GG_STR("a"), GG_STR("c")));
    TEST_ASSERT(test_buf_compare(GG_STR("b"), GG_STR("c")));
    TEST_ASSERT(test_buf_compare(GG_STR("abc"), GG_STR("cab")));
    TEST_ASSERT(test_buf_compare(GG_STR("abc"), GG_STR("abcd")));
    TEST_ASSERT_FALSE(test_buf_compare(GG_STR("cab"), GG_STR("abc")));
    TEST_ASSERT_FALSE(test_buf_compare(GG_STR("abc"), GG_STR("abc")));
    TEST_ASSERT_FALSE(test_buf_compare(GG_STR("abcd"), GG_STR("abc")));
}

GG_TEST_DEFINE(map_is_canonical) {
    TEST_ASSERT(gg_map_is_canonical(GG_MAP()));

    TEST_ASSERT(gg_map_is_canonical(GG_MAP(gg_kv(GG_STR("a"), GG_OBJ_NULL))));

    TEST_ASSERT(gg_map_is_canonical(GG_MAP(
        gg_kv(GG_STR("a"), GG_OBJ_NULL),
        gg_kv(GG_STR("b"), GG_OBJ_NULL),
        gg_kv(GG_STR("c"), GG_OBJ_NULL),
    )));

    // Contains duplicates
    TEST_ASSERT_FALSE(gg_map_is_canonical(
        GG_MAP(gg_kv(GG_STR("a"), GG_OBJ_NULL), gg_kv(GG_STR("a"), GG_OBJ_NULL))
    ));

    // Not sorted
    TEST_ASSERT_FALSE(gg_map_is_canonical(
        GG_MAP(gg_kv(GG_STR("b"), GG_OBJ_NULL), gg_kv(GG_STR("a"), GG_OBJ_NULL))
    ));
}

static void gg_test_prune_duplicates(
    GgMap expected, GgMap test_value, UNITY_UINT line_no
) {
    prune_duplicates(&test_value);
    gg_test_assert_map_equal(expected, test_value, NULL, line_no);
}

#define GG_TEST_PRUNE_DUPLICATES(expected, actual) \
    gg_test_prune_duplicates((expected), (actual), __LINE__)

GG_TEST_DEFINE(map_prune_duplicates) {
    GG_TEST_PRUNE_DUPLICATES(GG_MAP(), GG_MAP());

    GgMap identity = GG_MAP(
        gg_kv(GG_STR("a"), gg_obj_i64(3)),
        gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
        gg_kv(GG_STR("c"), gg_obj_bool(false))
    );
    GG_TEST_PRUNE_DUPLICATES(identity, identity);
    GG_TEST_PRUNE_DUPLICATES(
        identity,
        GG_MAP(
            gg_kv(GG_STR("a"), gg_obj_i64(3)),
            gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
            gg_kv(GG_STR("c"), gg_obj_bool(false)),
            gg_kv(GG_STR("c"), gg_obj_i64(2))
        )
    );
    GG_TEST_PRUNE_DUPLICATES(
        identity,
        GG_MAP(
            gg_kv(GG_STR("a"), gg_obj_i64(3)),
            gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
            gg_kv(GG_STR("c"), gg_obj_bool(false)),
            gg_kv(GG_STR("b"), gg_obj_i64(2))
        )
    );
    GG_TEST_PRUNE_DUPLICATES(
        identity,
        GG_MAP(
            gg_kv(GG_STR("a"), gg_obj_i64(3)),
            gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
            gg_kv(GG_STR("b"), gg_obj_i64(2)),
            gg_kv(GG_STR("c"), gg_obj_bool(false))
        )
    );

    GgMap unstable = GG_MAP(
        gg_kv(GG_STR("a"), gg_obj_i64(3)),
        gg_kv(GG_STR("c"), gg_obj_bool(false)),
        gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
    );
    GG_TEST_PRUNE_DUPLICATES(
        unstable,
        GG_MAP(
            gg_kv(GG_STR("a"), gg_obj_i64(3)),
            gg_kv(GG_STR("a"), gg_obj_i64(2)),
            gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
            gg_kv(GG_STR("c"), gg_obj_bool(false)),
        )
    );
}

static void gg_test_map_sort(GgMap expected, GgMap actual, UNITY_UINT line_no) {
    sort_keys(&actual);
    gg_test_assert_map_equal(expected, actual, NULL, line_no);
}

#define GG_TEST_MAP_SORT(expected, actual) \
    gg_test_map_sort(expected, actual, __LINE__)

GG_TEST_DEFINE(map_sorting) {
    GG_TEST_MAP_SORT(GG_MAP(), GG_MAP());

    GgMap sorted = GG_MAP(
        gg_kv(GG_STR("a"), gg_obj_i64(1)),
        gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
        gg_kv(GG_STR("c"), gg_obj_bool(false))
    );

    GG_TEST_MAP_SORT(sorted, sorted);
    GG_TEST_MAP_SORT(
        sorted,
        GG_MAP(
            gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
            gg_kv(GG_STR("a"), gg_obj_i64(1)),

            gg_kv(GG_STR("c"), gg_obj_bool(false))
        )
    );

    GG_TEST_MAP_SORT(
        sorted,
        GG_MAP(
            gg_kv(GG_STR("c"), gg_obj_bool(false)),
            gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
            gg_kv(GG_STR("a"), gg_obj_i64(1)),

        )
    );
    GG_TEST_MAP_SORT(
        sorted,
        GG_MAP(
            gg_kv(GG_STR("a"), gg_obj_i64(1)),
            gg_kv(GG_STR("c"), gg_obj_bool(false)),
            gg_kv(GG_STR("b"), gg_obj_f64(1.0)),

        )
    );

    GG_TEST_MAP_SORT(
        sorted,
        GG_MAP(
            gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
            gg_kv(GG_STR("c"), gg_obj_bool(false)),
            gg_kv(GG_STR("a"), gg_obj_i64(1)),

        )
    );

    GgMap sorted_stable = GG_MAP(
        gg_kv(GG_STR("a"), gg_obj_i64(1)),
        gg_kv(GG_STR("a"), gg_obj_f64(1)),
        gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
        gg_kv(GG_STR("b"), gg_obj_bool(true)),
        gg_kv(GG_STR("c"), gg_obj_bool(false)),
        gg_kv(GG_STR("c"), gg_obj_buf(GG_STR(""))),
    );

    GG_TEST_MAP_SORT(
        sorted_stable,
        GG_MAP(
            gg_kv(GG_STR("c"), gg_obj_bool(false)),
            gg_kv(GG_STR("a"), gg_obj_i64(1)),
            gg_kv(GG_STR("a"), gg_obj_f64(1)),
            gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
            gg_kv(GG_STR("b"), gg_obj_bool(true)),
            gg_kv(GG_STR("c"), gg_obj_buf(GG_STR(""))),
        )
    );

    GG_TEST_MAP_SORT(
        sorted_stable,
        GG_MAP(
            gg_kv(GG_STR("a"), gg_obj_i64(1)),
            gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
            gg_kv(GG_STR("b"), gg_obj_bool(true)),
            gg_kv(GG_STR("c"), gg_obj_bool(false)),
            gg_kv(GG_STR("c"), gg_obj_buf(GG_STR(""))),
            gg_kv(GG_STR("a"), gg_obj_f64(1)),
        )
    );

    GG_TEST_MAP_SORT(
        sorted_stable,
        GG_MAP(
            gg_kv(GG_STR("a"), gg_obj_i64(1)),
            gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
            gg_kv(GG_STR("c"), gg_obj_bool(false)),
            gg_kv(GG_STR("c"), gg_obj_buf(GG_STR(""))),
            gg_kv(GG_STR("a"), gg_obj_f64(1)),
            gg_kv(GG_STR("b"), gg_obj_bool(true)),
        )
    );
}

static void gg_test_map_canonicalize(
    GgMap expected, GgMap actual, UNITY_UINT line_no
) {
    gg_map_canonicalize_shallow(&actual);

    gg_test_assert_map_equal(expected, actual, NULL, line_no);
}

#define GG_TEST_MAP_CANONICALIZE(expected, actual) \
    gg_test_map_canonicalize((expected), (actual), __LINE__)

GG_TEST_DEFINE(map_canonicalize) {
    GG_TEST_MAP_CANONICALIZE(GG_MAP(), GG_MAP());

    GgMap canon = GG_MAP(
        gg_kv(GG_STR("a"), gg_obj_i64(1)),
        gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
        gg_kv(GG_STR("c"), gg_obj_bool(false))
    );

    GG_TEST_MAP_CANONICALIZE(
        canon,
        GG_MAP(
            gg_kv(GG_STR("c"), gg_obj_bool(false)),
            gg_kv(GG_STR("a"), gg_obj_i64(1)),
            gg_kv(GG_STR("b"), gg_obj_f64(1.0)),
            gg_kv(GG_STR("c"), gg_obj_buf(GG_STR(""))),
            gg_kv(GG_STR("a"), gg_obj_f64(1)),
            gg_kv(GG_STR("b"), gg_obj_bool(true)),
        )
    );
}
#endif
