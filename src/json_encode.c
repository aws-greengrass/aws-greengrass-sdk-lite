// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <float.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/io.h>
#include <ggl/json_encode.h>
#include <ggl/log.h>
#include <ggl/object.h>
#include <ggl/object_visit.h>
#include <ggl/vector.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

static GglError json_encode_on_null(void *ctx) {
    GglWriter *writer = ctx;
    return ggl_writer_call(*writer, GGL_STR("null"));
}

static GglError json_encode_on_bool(void *ctx, bool val) {
    GglWriter *writer = ctx;
    return ggl_writer_call(*writer, val ? GGL_STR("true") : GGL_STR("false"));
}

static GglError json_encode_on_i64(void *ctx, int64_t val) {
    GglWriter *writer = ctx;
    // (size_t) (  ( CHAR_BITS * sizeof(int64_t) - 1 )  * log10( 2.0 ) ) + 1U
    char encoded[19];
    int ret_len = snprintf(encoded, sizeof(encoded), "%" PRId64, val);
    if (ret_len < 0) {
        GGL_LOGE("Error encoding json.");
        return GGL_ERR_FAILURE;
    }
    if ((size_t) ret_len > sizeof(encoded)) {
        assert((size_t) ret_len <= sizeof(encoded));
        return GGL_ERR_NOMEM;
    }
    return ggl_writer_call(
        *writer,
        (GglBuffer) { .data = (uint8_t *) encoded, .len = (size_t) ret_len }
    );
}

static GglError json_encode_on_f64(void *ctx, double val) {
    GglWriter *writer = ctx;
    char encoded[DBL_DECIMAL_DIG + 9]; // -x.<precision>E-xxx\0
    int ret_len
        = snprintf(encoded, sizeof(encoded), "%#.*g", DBL_DECIMAL_DIG, val);
    if (ret_len < 0) {
        GGL_LOGE("Error encoding json.");
        return GGL_ERR_FAILURE;
    }
    if ((size_t) ret_len > sizeof(encoded)) {
        assert((size_t) ret_len <= sizeof(encoded));
        return GGL_ERR_NOMEM;
    }
    return ggl_writer_call(
        *writer,
        (GglBuffer) { .data = (uint8_t *) encoded, .len = (size_t) ret_len }
    );
}

static GglError json_write_buf_byte(uint8_t byte, GglWriter writer) {
    GglError ret = GGL_ERR_OK;
    if ((char) byte == '"') {
        ret = ggl_writer_call(writer, GGL_STR("\\\""));
        if (ret != GGL_ERR_OK) {
            return ret;
        }
    } else if ((char) byte == '\\') {
        ret = ggl_writer_call(writer, GGL_STR("\\\\"));
        if (ret != GGL_ERR_OK) {
            return ret;
        }
    } else if (byte <= 0x001F) {
        uint8_t encoded_byte[7];
        int ret_len = snprintf(
            (char *) encoded_byte, sizeof(encoded_byte), "\\u00%02X", byte
        );
        if (ret_len < 0) {
            GGL_LOGE("Error encoding json.");
            return GGL_ERR_FAILURE;
        }
        if ((size_t) ret_len > sizeof(encoded_byte)) {
            assert((size_t) ret_len <= sizeof(encoded_byte));
            return GGL_ERR_NOMEM;
        }
        ret = ggl_writer_call(
            writer,
            (GglBuffer) { .data = encoded_byte, .len = (size_t) ret_len }
        );
        if (ret != GGL_ERR_OK) {
            return ret;
        }
    } else {
        ret = ggl_writer_call(writer, (GglBuffer) { .data = &byte, .len = 1 });
        if (ret != GGL_ERR_OK) {
            return ret;
        }
    }
    return ret;
}

static GglError json_encode_on_buf(void *ctx, GglBuffer val, GglObject *obj) {
    GglWriter *writer = ctx;
    (void) obj;

    GglError ret = ggl_writer_call(*writer, GGL_STR("\""));
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    for (size_t i = 0; i < val.len; i++) {
        ret = json_write_buf_byte(val.data[i], *writer);
        if (ret != GGL_ERR_OK) {
            return ret;
        }
    }

    ret = ggl_writer_call(*writer, GGL_STR("\""));
    if (ret != GGL_ERR_OK) {
        return ret;
    }
    return GGL_ERR_OK;
}

static GglError json_encode_on_list(void *ctx, GglList val, GglObject *obj) {
    GglWriter *writer = ctx;
    (void) obj;
    (void) val;
    return ggl_writer_call(*writer, GGL_STR("["));
}

static GglError json_encode_cont_list(void *ctx) {
    GglWriter *writer = ctx;
    return ggl_writer_call(*writer, GGL_STR(","));
}

static GglError json_encode_end_list(void *ctx) {
    GglWriter *writer = ctx;
    return ggl_writer_call(*writer, GGL_STR("]"));
}

static GglError json_encode_on_map(void *ctx, GglMap val, GglObject *obj) {
    GglWriter *writer = ctx;
    (void) obj;
    (void) val;
    return ggl_writer_call(*writer, GGL_STR("{"));
}

static GglError json_encode_on_map_key(void *ctx, GglBuffer key, GglKV *kv) {
    GglWriter *writer = ctx;
    (void) kv;
    GglError ret = json_encode_on_buf(ctx, key, NULL);
    if (ret != GGL_ERR_OK) {
        return ret;
    }
    return ggl_writer_call(*writer, GGL_STR(":"));
}

static GglError json_encode_cont_map(void *ctx) {
    GglWriter *writer = ctx;
    return ggl_writer_call(*writer, GGL_STR(","));
}

static GglError json_encode_end_map(void *ctx) {
    GglWriter *writer = ctx;
    return ggl_writer_call(*writer, GGL_STR("}"));
}

GglError ggl_json_encode(GglObject obj, GglWriter writer) {
    const GglObjectVisitHandlers VISIT_HANDLERS = {
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
    return ggl_obj_visit(&VISIT_HANDLERS, &writer, &obj);
}

static GglError obj_read(void *ctx, GglBuffer *buf) {
    assert(buf != NULL);

    const GglObject *obj = ctx;

    if ((obj == NULL) || (buf == NULL)) {
        return GGL_ERR_INVALID;
    }

    GglByteVec vec = ggl_byte_vec_init(*buf);
    GglError ret = ggl_json_encode(*obj, ggl_byte_vec_writer(&vec));
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    *buf = vec.buf;
    return GGL_ERR_OK;
}

GglReader ggl_json_reader(const GglObject *obj) {
    assert(obj != NULL);
    return (GglReader) { .read = obj_read, .ctx = (void *) obj };
}
