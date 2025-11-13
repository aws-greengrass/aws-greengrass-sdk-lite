// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
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
