// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <float.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/io.h>
#include <gg/json_encode.h>
#include <gg/log.h>
#include <gg/object.h>
#include <gg/object_visit.h>
#include <gg/vector.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

static GgError json_encode_on_null(void *ctx) {
    GgWriter *writer = ctx;
    return gg_writer_call(*writer, GG_STR("null"));
}

static GgError json_encode_on_bool(void *ctx, bool val) {
    GgWriter *writer = ctx;
    return gg_writer_call(*writer, val ? GG_STR("true") : GG_STR("false"));
}

static GgError json_encode_on_i64(void *ctx, int64_t val) {
    GgWriter *writer = ctx;
    // (size_t) (  ( CHAR_BITS * sizeof(int64_t) - 1 )  * log10( 2.0 ) ) + 1U
    char encoded[19];
    int ret_len = snprintf(encoded, sizeof(encoded), "%" PRId64, val);
    if (ret_len < 0) {
        GG_LOGE("Error encoding json.");
        return GG_ERR_FAILURE;
    }
    if ((size_t) ret_len > sizeof(encoded)) {
        assert((size_t) ret_len <= sizeof(encoded));
        return GG_ERR_NOMEM;
    }
    return gg_writer_call(
        *writer,
        (GgBuffer) { .data = (uint8_t *) encoded, .len = (size_t) ret_len }
    );
}

static GgError json_encode_on_f64(void *ctx, double val) {
    GgWriter *writer = ctx;
    char encoded[DBL_DECIMAL_DIG + 9]; // -x.<precision>E-xxx\0
    int ret_len
        = snprintf(encoded, sizeof(encoded), "%#.*g", DBL_DECIMAL_DIG, val);
    if (ret_len < 0) {
        GG_LOGE("Error encoding json.");
        return GG_ERR_FAILURE;
    }
    if ((size_t) ret_len > sizeof(encoded)) {
        assert((size_t) ret_len <= sizeof(encoded));
        return GG_ERR_NOMEM;
    }
    return gg_writer_call(
        *writer,
        (GgBuffer) { .data = (uint8_t *) encoded, .len = (size_t) ret_len }
    );
}

static GgError json_write_buf_byte(uint8_t byte, GgWriter writer) {
    GgError ret = GG_ERR_OK;
    if ((char) byte == '"') {
        ret = gg_writer_call(writer, GG_STR("\\\""));
        if (ret != GG_ERR_OK) {
            return ret;
        }
    } else if ((char) byte == '\\') {
        ret = gg_writer_call(writer, GG_STR("\\\\"));
        if (ret != GG_ERR_OK) {
            return ret;
        }
    } else if (byte <= 0x001F) {
        uint8_t encoded_byte[7];
        int ret_len = snprintf(
            (char *) encoded_byte, sizeof(encoded_byte), "\\u00%02X", byte
        );
        if (ret_len < 0) {
            GG_LOGE("Error encoding json.");
            return GG_ERR_FAILURE;
        }
        if ((size_t) ret_len > sizeof(encoded_byte)) {
            assert((size_t) ret_len <= sizeof(encoded_byte));
            return GG_ERR_NOMEM;
        }
        ret = gg_writer_call(
            writer, (GgBuffer) { .data = encoded_byte, .len = (size_t) ret_len }
        );
        if (ret != GG_ERR_OK) {
            return ret;
        }
    } else {
        ret = gg_writer_call(writer, (GgBuffer) { .data = &byte, .len = 1 });
        if (ret != GG_ERR_OK) {
            return ret;
        }
    }
    return ret;
}

static GgError json_encode_on_buf(void *ctx, GgBuffer val, GgObject *obj) {
    GgWriter *writer = ctx;
    (void) obj;

    GgError ret = gg_writer_call(*writer, GG_STR("\""));
    if (ret != GG_ERR_OK) {
        return ret;
    }

    for (size_t i = 0; i < val.len; i++) {
        ret = json_write_buf_byte(val.data[i], *writer);
        if (ret != GG_ERR_OK) {
            return ret;
        }
    }

    ret = gg_writer_call(*writer, GG_STR("\""));
    if (ret != GG_ERR_OK) {
        return ret;
    }
    return GG_ERR_OK;
}

static GgError json_encode_on_list(void *ctx, GgList val, GgObject *obj) {
    GgWriter *writer = ctx;
    (void) obj;
    (void) val;
    return gg_writer_call(*writer, GG_STR("["));
}

static GgError json_encode_cont_list(void *ctx) {
    GgWriter *writer = ctx;
    return gg_writer_call(*writer, GG_STR(","));
}

static GgError json_encode_end_list(void *ctx) {
    GgWriter *writer = ctx;
    return gg_writer_call(*writer, GG_STR("]"));
}

static GgError json_encode_on_map(void *ctx, GgMap val, GgObject *obj) {
    GgWriter *writer = ctx;
    (void) obj;
    (void) val;
    return gg_writer_call(*writer, GG_STR("{"));
}

static GgError json_encode_on_map_key(void *ctx, GgBuffer key, GgKV *kv) {
    GgWriter *writer = ctx;
    (void) kv;
    GgError ret = json_encode_on_buf(ctx, key, NULL);
    if (ret != GG_ERR_OK) {
        return ret;
    }
    return gg_writer_call(*writer, GG_STR(":"));
}

static GgError json_encode_cont_map(void *ctx) {
    GgWriter *writer = ctx;
    return gg_writer_call(*writer, GG_STR(","));
}

static GgError json_encode_end_map(void *ctx) {
    GgWriter *writer = ctx;
    return gg_writer_call(*writer, GG_STR("}"));
}

GgError gg_json_encode(GgObject obj, GgWriter writer) {
    const GgObjectVisitHandlers VISIT_HANDLERS = {
        .on_null = json_encode_on_null,
        .on_bool = json_encode_on_bool,
        .on_i64 = json_encode_on_i64,
        .on_f64 = json_encode_on_f64,
        .on_buf = json_encode_on_buf,
        .on_list = json_encode_on_list,
        .cont_list = json_encode_cont_list,
        .end_list = json_encode_end_list,
        .on_map = json_encode_on_map,
        .on_map_key = json_encode_on_map_key,
        .cont_map = json_encode_cont_map,
        .end_map = json_encode_end_map,
    };
    return gg_obj_visit(&VISIT_HANDLERS, &writer, &obj);
}

static GgError obj_read(void *ctx, GgBuffer *buf) {
    assert(buf != NULL);

    const GgObject *obj = ctx;

    if ((obj == NULL) || (buf == NULL)) {
        return GG_ERR_INVALID;
    }

    GgByteVec vec = gg_byte_vec_init(*buf);
    GgError ret = gg_json_encode(*obj, gg_byte_vec_writer(&vec));
    if (ret != GG_ERR_OK) {
        return ret;
    }

    *buf = vec.buf;
    return GG_ERR_OK;
}

GgReader gg_json_reader(const GgObject *obj) {
    assert(obj != NULL);
    return (GgReader) { .read = obj_read, .ctx = (void *) obj };
}

#ifdef GG_TESTING

#include <gg/map.h>
#include <gg/test.h>
#include <unity.h>
#include <stdlib.h>

GG_TEST_DEFINE(json_encode_null_ok) {
    GgObject null = GG_OBJ_NULL;
    GgBuffer buf = GG_BUF((uint8_t[4]) { 0 });
    GgByteVec vec = gg_byte_vec_init(buf);
    GG_TEST_ASSERT_OK(gg_json_encode(null, gg_byte_vec_writer(&vec)));
    GG_TEST_ASSERT_BUF_EQUAL_STR(GG_STR("null"), vec.buf);
}

GG_TEST_DEFINE(json_encode_bool_ok) {
    {
        GgObject obj_false = gg_obj_bool(false);
        GgBuffer buf = GG_BUF((uint8_t[5]) { 0 });
        GgByteVec vec = gg_byte_vec_init(buf);
        GG_TEST_ASSERT_OK(gg_json_encode(obj_false, gg_byte_vec_writer(&vec)));
        GG_TEST_ASSERT_BUF_EQUAL_STR(GG_STR("false"), vec.buf);
    }

    {
        GgObject obj_true = gg_obj_bool(true);
        GgBuffer buf = GG_BUF((uint8_t[4]) { 0 });
        GgByteVec vec = gg_byte_vec_init(buf);
        GG_TEST_ASSERT_OK(gg_json_encode(obj_true, gg_byte_vec_writer(&vec)));
        GG_TEST_ASSERT_BUF_EQUAL_STR(GG_STR("true"), vec.buf);
    }
}

GG_TEST_DEFINE(json_encode_i64_ok) {
    GgObject obj = gg_obj_i64(123);
    GgBuffer buf = GG_BUF((uint8_t[3]) { 0 });
    GgByteVec vec = gg_byte_vec_init(buf);
    GG_TEST_ASSERT_OK(gg_json_encode(obj, gg_byte_vec_writer(&vec)));
    GG_TEST_ASSERT_BUF_EQUAL_STR(GG_STR("123"), vec.buf);
}

GG_TEST_DEFINE(json_encode_f64_ok) {
    GgObject obj = gg_obj_f64(123.456);
    GgBuffer buf = GG_BUF((uint8_t[DBL_DECIMAL_DIG + 9 + 1]) { 0 });
    buf.len -= 1;
    GgByteVec vec = gg_byte_vec_init(buf);
    GG_TEST_ASSERT_OK(gg_json_encode(obj, gg_byte_vec_writer(&vec)));

    // Only feasible to test that round-tripping results in a similar value
    TEST_ASSERT_EQUAL_DOUBLE(123.456, strtod((char *) buf.data, NULL));
}

GG_TEST_DEFINE(json_encode_buf_ok) {
    {
        GgObject obj = gg_obj_buf(GG_BUF((uint8_t[1]) { 0x1F }));
        GgBuffer buf = GG_BUF((uint8_t[8]) { 0 });
        GgByteVec vec = gg_byte_vec_init(buf);
        GG_TEST_ASSERT_OK(gg_json_encode(obj, gg_byte_vec_writer(&vec)));
        GG_TEST_ASSERT_BUF_EQUAL_STR(GG_STR("\"\\u001F\""), vec.buf);
    }

    {
        GgObject obj = gg_obj_buf(GG_STR("Hello, world!"));
        GgBuffer buf = GG_BUF((uint8_t[15]) { 0 });
        GgByteVec vec = gg_byte_vec_init(buf);
        GG_TEST_ASSERT_OK(gg_json_encode(obj, gg_byte_vec_writer(&vec)));
        GG_TEST_ASSERT_BUF_EQUAL_STR(GG_STR("\"Hello, world!\""), vec.buf);
    }

    {
        GgObject obj = gg_obj_buf(GG_STR("\"escape \\ me \" \0"));
        GgBuffer buf = GG_BUF((uint8_t[26]) { 0 });
        GgByteVec vec = gg_byte_vec_init(buf);
        GG_TEST_ASSERT_OK(gg_json_encode(obj, gg_byte_vec_writer(&vec)));
        GG_TEST_ASSERT_BUF_EQUAL_STR(
            GG_STR("\"\\\"escape \\\\ me \\\" \\u0000\""), vec.buf
        );
    }
}

GG_TEST_DEFINE(json_encode_map_ok) {
    {
        GgObject obj = gg_obj_map(GG_MAP(
            gg_kv(GG_STR("a"), gg_obj_i64(1)),
            gg_kv(GG_STR("b"), gg_obj_i64(2)),
            gg_kv(GG_STR("c"), gg_obj_i64(3))
        ));
        GgBuffer buf = GG_BUF((uint8_t[20]) { 0 });
        GgByteVec vec = gg_byte_vec_init(buf);
        GG_TEST_ASSERT_OK(gg_json_encode(obj, gg_byte_vec_writer(&vec)));
        GG_TEST_ASSERT_BUF_EQUAL_STR(
            GG_STR("{\"a\":1,\"b\":2,\"c\":3}"), vec.buf
        );
    }
}

GG_TEST_DEFINE(json_encode_list_ok) {
    {
        GgObject obj
            = gg_obj_list(GG_LIST(gg_obj_i64(1), gg_obj_i64(2), gg_obj_i64(3)));
        GgBuffer buf = GG_BUF((uint8_t[7]) { 0 });
        GgByteVec vec = gg_byte_vec_init(buf);
        GG_TEST_ASSERT_OK(gg_json_encode(obj, gg_byte_vec_writer(&vec)));
        GG_TEST_ASSERT_BUF_EQUAL_STR(GG_STR("[1,2,3]"), vec.buf);
    }
}

GG_TEST_DEFINE(json_encode_map_too_nested) {
    GgKV pairs[GG_MAX_OBJECT_DEPTH + 1];
    GgObject too_nested[GG_MAX_OBJECT_DEPTH + 1];
    // {"a"={"a"={...}}}
    size_t too_nested_len = sizeof(too_nested) / sizeof(too_nested[0]);
    static_assert(
        sizeof(too_nested) / sizeof(too_nested[0])
            == sizeof(pairs) / sizeof(pairs[0]),
        "size mismatch"
    );
    for (size_t i = 0; i != too_nested_len; i++) {
        pairs[i] = gg_kv(GG_STR("a"), GG_OBJ_NULL);
        too_nested[i] = gg_obj_map((GgMap) { .pairs = &pairs[i], .len = 1 });
    }
    for (size_t i = 0; i != too_nested_len - 1; ++i) {
        *gg_kv_val(&pairs[i]) = too_nested[i + 1];
    }

    GgBuffer buf = GG_BUF((uint8_t[16 *(GG_MAX_OBJECT_DEPTH + 1)]) { 0 });
    GgByteVec vec = gg_byte_vec_init(buf);
    GgError ret = gg_json_encode(too_nested[0], gg_byte_vec_writer(&vec));
    TEST_ASSERT_EQUAL(GG_ERR_RANGE, ret);
}

GG_TEST_DEFINE(json_encode_map_too_large) {
    GgKV too_large[GG_MAX_OBJECT_SUBOBJECTS / 2 + 1];
    // {"a"=0,"a"=1,...}
    size_t too_large_len = sizeof(too_large) / sizeof(too_large[0]);
    for (size_t i = 0; i != too_large_len; i++) {
        too_large[i] = gg_kv(GG_STR("a"), gg_obj_i64((int64_t) i));
    }

    GgBuffer buf
        = GG_BUF((uint8_t[16 *(GG_MAX_OBJECT_SUBOBJECTS / 2 + 1)]) { 0 });
    GgByteVec vec = gg_byte_vec_init(buf);
    GgError ret = gg_json_encode(
        gg_obj_map((GgMap) { .pairs = too_large, .len = too_large_len }),
        gg_byte_vec_writer(&vec)
    );
    TEST_ASSERT_EQUAL(GG_ERR_RANGE, ret);
}

GG_TEST_DEFINE(json_encode_list_too_nested) {
    GgObject too_nested[GG_MAX_OBJECT_DEPTH + 1];
    // [[[...[null]...]]]
    size_t too_nested_len = sizeof(too_nested) / sizeof(too_nested[0]);
    for (size_t i = 0; i != too_nested_len - 1; i++) {
        too_nested[i]
            = gg_obj_list((GgList) { .items = &too_nested[i + 1], .len = 1 });
    }
    too_nested[too_nested_len - 1] = GG_OBJ_NULL;

    GgBuffer buf = GG_BUF((uint8_t[16 *(GG_MAX_OBJECT_DEPTH + 1)]) { 0 });
    GgByteVec vec = gg_byte_vec_init(buf);
    GgError ret = gg_json_encode(too_nested[0], gg_byte_vec_writer(&vec));
    TEST_ASSERT_EQUAL(GG_ERR_RANGE, ret);
}

GG_TEST_DEFINE(json_encode_list_too_large) {
    GgObject too_large[GG_MAX_OBJECT_SUBOBJECTS + 1];
    // [null,null,null,...,null]
    size_t too_large_len = sizeof(too_large) / sizeof(too_large[0]);
    for (size_t i = 0; i != too_large_len; i++) {
        too_large[i] = GG_OBJ_NULL;
    }

    GgBuffer buf = GG_BUF((uint8_t[16 *(GG_MAX_OBJECT_SUBOBJECTS + 1)]) { 0 });

    GgByteVec vec = gg_byte_vec_init(buf);
    GgError ret = gg_json_encode(
        gg_obj_list((GgList) { .items = too_large, .len = too_large_len }),
        gg_byte_vec_writer(&vec)
    );
    TEST_ASSERT_EQUAL(GG_ERR_RANGE, ret);
}

#endif
