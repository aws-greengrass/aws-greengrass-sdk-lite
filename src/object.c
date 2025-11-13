// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/log.h>
#include <gg/object.h>
#include <gg/object_visit.h>
#include <string.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>

static_assert(
    sizeof(void *) == sizeof(size_t),
    "Platforms where pointers and size_t are not same width are not currently supported."
);

static_assert(
    (sizeof(size_t) == 4) || (sizeof(size_t) == 8),
    "Only 32 or 64-bit platforms are supported."
);

GgObjectType gg_obj_type(GgObject obj) {
    // Last byte is tag
    uint8_t result = obj._private[sizeof(obj._private) - 1];
    assert(result <= 6);
    return (GgObjectType) result;
}

GgObject gg_obj_bool(bool value) {
    GgObject result = { 0 };
    static_assert(
        sizeof(result._private) >= sizeof(value) + 1,
        "Object must be able to hold bool and tag."
    );
    memcpy(result._private, &value, sizeof(value));
    result._private[sizeof(result._private) - 1] = GG_TYPE_BOOLEAN;
    return result;
}

bool gg_obj_into_bool(GgObject boolean) {
    assert(gg_obj_type(boolean) == GG_TYPE_BOOLEAN);
    bool result;
    memcpy(&result, boolean._private, sizeof(result));
    return result;
}

GgObject gg_obj_i64(int64_t value) {
    GgObject result = { 0 };
    static_assert(
        sizeof(result._private) >= sizeof(value) + 1,
        "GgObject must be able to hold int64_t and tag."
    );
    memcpy(result._private, &value, sizeof(value));
    result._private[sizeof(result._private) - 1] = GG_TYPE_I64;
    return result;
}

int64_t gg_obj_into_i64(GgObject i64) {
    assert(gg_obj_type(i64) == GG_TYPE_I64);
    int64_t result;
    memcpy(&result, i64._private, sizeof(result));
    return result;
}

GgObject gg_obj_f64(double value) {
    GgObject result = { 0 };
    static_assert(
        sizeof(result._private) >= sizeof(value) + 1,
        "GgObject must be able to hold double and tag."
    );
    memcpy(result._private, &value, sizeof(value));
    result._private[sizeof(result._private) - 1] = GG_TYPE_F64;
    return result;
}

double gg_obj_into_f64(GgObject f64) {
    assert(gg_obj_type(f64) == GG_TYPE_F64);
    double result;
    memcpy(&result, f64._private, sizeof(result));
    return result;
}

COLD
static void length_err(const char *type, size_t *len) {
    GG_LOGE(
        "%s length longer than can be stored in GgObject (%zu, max %u).",
        type,
        *len,
        (unsigned int) UINT16_MAX
    );
    assert(false);
    *len = UINT16_MAX;
}

GgObject gg_obj_buf(GgBuffer value) {
    if (value.len > UINT16_MAX) {
        length_err("GgBuffer", &value.len);
    }
    uint16_t len = (uint16_t) value.len;

    GgObject result = { 0 };
    static_assert(
        sizeof(result._private) >= sizeof(void *) + 2 + 1,
        "GgObject must be able to hold pointer + 16-bit len + tag."
    );
    memcpy(result._private, &value.data, sizeof(void *));
    memcpy(&result._private[sizeof(void *)], &len, 2);
    result._private[sizeof(result._private) - 1] = GG_TYPE_BUF;
    return result;
}

GgBuffer gg_obj_into_buf(GgObject buf) {
    assert(gg_obj_type(buf) == GG_TYPE_BUF);
    void *ptr;
    uint16_t len;
    memcpy(&ptr, buf._private, sizeof(void *));
    memcpy(&len, &buf._private[sizeof(void *)], 2);
    return (GgBuffer) { .data = ptr, .len = len };
}

GgObject gg_obj_map(GgMap value) {
    if (value.len > UINT16_MAX) {
        length_err("GgMap", &value.len);
    }
    uint16_t len = (uint16_t) value.len;

    GgObject result = { 0 };
    static_assert(
        sizeof(result._private) >= sizeof(void *) + 2 + 1,
        "GgObject must be able to hold pointer + 16-bit len + tag."
    );
    memcpy(result._private, &value.pairs, sizeof(void *));
    memcpy(&result._private[sizeof(void *)], &len, 2);
    result._private[sizeof(result._private) - 1] = GG_TYPE_MAP;
    return result;
}

GgMap gg_obj_into_map(GgObject map) {
    assert(gg_obj_type(map) == GG_TYPE_MAP);
    void *ptr;
    uint16_t len;
    memcpy(&ptr, map._private, sizeof(void *));
    memcpy(&len, &map._private[sizeof(void *)], 2);
    return (GgMap) { .pairs = ptr, .len = len };
}

GgObject gg_obj_list(GgList value) {
    if (value.len > UINT16_MAX) {
        length_err("GgList", &value.len);
    }
    uint16_t len = (uint16_t) value.len;

    GgObject result = { 0 };
    static_assert(
        sizeof(result._private) >= sizeof(void *) + 2 + 1,
        "GgObject must be able to hold pointer + 16-bit len + tag."
    );
    memcpy(result._private, &value.items, sizeof(void *));
    memcpy(&result._private[sizeof(void *)], &len, sizeof(len));
    result._private[sizeof(result._private) - 1] = GG_TYPE_LIST;
    return result;
}

GgList gg_obj_into_list(GgObject list) {
    assert(gg_obj_type(list) == GG_TYPE_LIST);
    void *ptr;
    uint16_t len;
    memcpy(&ptr, list._private, sizeof(void *));
    memcpy(&len, &list._private[sizeof(void *)], 2);
    return (GgList) { .items = ptr, .len = len };
}

static GgError mem_usage_buf(void *ctx, GgBuffer val, GgObject obj[static 1]) {
    (void) obj;
    size_t *measured = ctx;
    *measured += val.len;
    return GG_ERR_OK;
}

static GgError mem_usage_list(void *ctx, GgList val, GgObject obj[static 1]) {
    (void) obj;
    size_t *measured = ctx;
    static_assert(alignof(GgObject) == 1, "Assumes no padding");
    *measured += val.len * sizeof(GgObject);
    return GG_ERR_OK;
}

static GgError mem_usage_map(void *ctx, GgMap val, GgObject obj[static 1]) {
    (void) obj;
    size_t *measured = ctx;
    static_assert(alignof(GgKV) == 1, "Assumes no padding");
    *measured += val.len * sizeof(GgKV);
    return GG_ERR_OK;
}

static GgError mem_usage_map_key(void *ctx, GgBuffer key, GgKV kv[static 1]) {
    (void) kv;
    size_t *measured = ctx;
    *measured += key.len;
    return GG_ERR_OK;
}

GgError gg_obj_mem_usage(GgObject obj, size_t *size) {
    const GgObjectVisitHandlers VISIT_HANDLERS = {
        .on_buf = mem_usage_buf,
        .on_map_key = mem_usage_map_key,
        .on_map = mem_usage_map,
        .on_list = mem_usage_list,
    };

    size_t measured = 0;
    GgError ret = gg_obj_visit(&VISIT_HANDLERS, &measured, &obj);
    if ((size != NULL) && (ret == GG_ERR_OK)) {
        *size = measured;
    }
    return ret;
}
