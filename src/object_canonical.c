#include "gg/object.h"
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/log.h>
#include <gg/map.h>
#include <gg/object_visit.h>
#include <stdbool.h>
#include <stddef.h>

static GgError canonicalize_map(void *ctx, GgMap val, GgObject obj[static 1]) {
    (void) ctx;

    gg_map_canonicalize_shallow(&val);

    *obj = gg_obj_map(val);
    return GG_ERR_OK;
}

GgError gg_obj_canonicalize(GgObject *obj) {
    if (obj == NULL) {
        return GG_ERR_OK;
    }

    GgObjectVisitHandlers handlers = { .on_map = canonicalize_map };

    return gg_obj_visit(&handlers, NULL, obj);
}

static GgError is_map_canonical(void *ctx, GgMap val, GgObject obj[static 1]) {
    (void) ctx;
    (void) obj;

    if (!gg_map_is_canonical(val)) {
        return GG_ERR_FAILURE;
    }

    return GG_ERR_OK;
}

bool gg_obj_is_canonical(GgObject obj) {
    GgObjectVisitHandlers handlers = { .on_map = is_map_canonical };
    return GG_ERR_OK == gg_obj_visit(&handlers, NULL, &obj);
}

#ifdef GG_SDK_TESTING

#include <gg/test.h>
#include <unity.h>

GG_TEST_DEFINE(obj_is_canonical_simple_types) {
    TEST_ASSERT(gg_obj_is_canonical(GG_OBJ_NULL));
    TEST_ASSERT(gg_obj_is_canonical(gg_obj_i64(1)));
    TEST_ASSERT(gg_obj_is_canonical(gg_obj_f64(1.0)));
    TEST_ASSERT(gg_obj_is_canonical(gg_obj_bool(true)));
    TEST_ASSERT(gg_obj_is_canonical(gg_obj_list(GG_LIST())));
    TEST_ASSERT(gg_obj_is_canonical(gg_obj_buf(GG_STR("1.0"))));
}

GG_TEST_DEFINE(obj_is_canonical_too_long) {
    // Too many subobjects
    GgKV long_pairs[GG_MAX_OBJECT_SUBOBJECTS / 2 + 1] = { 0 };
    uint8_t long_keys[GG_MAX_OBJECT_SUBOBJECTS / 2 + 1][2];
    size_t long_pairs_len = sizeof(long_pairs) / sizeof(long_pairs[0]);
    for (uint8_t i = 0; i != long_pairs_len; ++i) {
        long_keys[i][0] = 'a' + (i / 16);
        long_keys[i][1] = 'a' + (i % 16);
        long_pairs[i] = gg_kv(GG_BUF(long_keys[i]), GG_OBJ_NULL);
    }
    TEST_ASSERT_FALSE(gg_obj_is_canonical(gg_obj_map((GgMap) {
        .pairs = long_pairs, .len = long_pairs_len })));
}

#endif
