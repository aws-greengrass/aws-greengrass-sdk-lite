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
#include <ggl/map.h>
#include <ggl/object.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

static GglError json_write(GglObject obj, GglWriter writer);

static GglError json_write_null(GglWriter writer) {
    return ggl_writer_call(writer, GGL_STR("null"));
}

static GglError json_write_bool(bool b, GglWriter writer) {
    return ggl_writer_call(writer, b ? GGL_STR("true") : GGL_STR("false"));
}

static GglError json_write_i64(int64_t i64, GglWriter writer) {
    // (size_t) (  ( CHAR_BITS * sizeof(int64_t) - 1 )  * log10( 2.0 ) ) + 1U
    char encoded[19];
    int ret_len = snprintf(encoded, sizeof(encoded), "%" PRId64, i64);
    if (ret_len < 0) {
        GGL_LOGE("Error encoding json.");
        return GGL_ERR_FAILURE;
    }
    if ((size_t) ret_len > sizeof(encoded)) {
        assert((size_t) ret_len <= sizeof(encoded));
        return GGL_ERR_NOMEM;
    }
    return ggl_writer_call(
        writer,
        (GglBuffer) { .data = (uint8_t *) encoded, .len = (size_t) ret_len }
    );
}

static GglError json_write_f64(double f64, GglWriter writer) {
    char encoded[DBL_DECIMAL_DIG + 4]; // -x.xxxxxxxE-xxxx
    int ret_len
        = snprintf(encoded, sizeof(encoded), "%#.*g", DBL_DECIMAL_DIG, f64);
    if (ret_len < 0) {
        GGL_LOGE("Error encoding json.");
        return GGL_ERR_FAILURE;
    }
    if ((size_t) ret_len > sizeof(encoded)) {
        assert((size_t) ret_len <= sizeof(encoded));
        return GGL_ERR_NOMEM;
    }
    return ggl_writer_call(
        writer,
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

static GglError json_write_buf(GglBuffer str, GglWriter writer) {
    GglError ret = ggl_writer_call(writer, GGL_STR("\""));
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    for (size_t i = 0; i < str.len; i++) {
        ret = json_write_buf_byte(str.data[i], writer);
        if (ret != GGL_ERR_OK) {
            return ret;
        }
    }

    ret = ggl_writer_call(writer, GGL_STR("\""));
    if (ret != GGL_ERR_OK) {
        return ret;
    }
    return GGL_ERR_OK;
}

// NOLINTNEXTLINE(misc-no-recursion)
static GglError json_write_list(GglList list, GglWriter writer) {
    GglError ret = ggl_writer_call(writer, GGL_STR("["));
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    for (size_t i = 0; i < list.len; i++) {
        if (i != 0) {
            ret = ggl_writer_call(writer, GGL_STR(","));
            if (ret != GGL_ERR_OK) {
                return ret;
            }
        }

        ret = json_write(list.items[i], writer);
        if (ret != GGL_ERR_OK) {
            return ret;
        }
    }

    ret = ggl_writer_call(writer, GGL_STR("]"));
    if (ret != GGL_ERR_OK) {
        return ret;
    }
    return GGL_ERR_OK;
}

// NOLINTNEXTLINE(misc-no-recursion)
static GglError json_write_map(GglMap map, GglWriter writer) {
    GglError ret = ggl_writer_call(writer, GGL_STR("{"));
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    for (size_t i = 0; i < map.len; i++) {
        if (i != 0) {
            ret = ggl_writer_call(writer, GGL_STR(","));
            if (ret != GGL_ERR_OK) {
                return ret;
            }
        }
        ret = json_write(ggl_obj_buf(ggl_kv_key(map.pairs[i])), writer);
        if (ret != GGL_ERR_OK) {
            return ret;
        }

        ret = ggl_writer_call(writer, GGL_STR(":"));
        if (ret != GGL_ERR_OK) {
            return ret;
        }

        ret = json_write(*ggl_kv_val(&map.pairs[i]), writer);
        if (ret != GGL_ERR_OK) {
            return ret;
        }
    }

    ret = ggl_writer_call(writer, GGL_STR("}"));
    if (ret != GGL_ERR_OK) {
        return ret;
    }
    return GGL_ERR_OK;
}

// NOLINTNEXTLINE(misc-no-recursion)
static GglError json_write(GglObject obj, GglWriter writer) {
    switch (ggl_obj_type(obj)) {
    case GGL_TYPE_NULL:
        return json_write_null(writer);
    case GGL_TYPE_BOOLEAN:
        return json_write_bool(ggl_obj_into_bool(obj), writer);
    case GGL_TYPE_I64:
        return json_write_i64(ggl_obj_into_i64(obj), writer);
    case GGL_TYPE_F64:
        return json_write_f64(ggl_obj_into_f64(obj), writer);
    case GGL_TYPE_BUF:
        return json_write_buf(ggl_obj_into_buf(obj), writer);
    case GGL_TYPE_LIST:
        return json_write_list(ggl_obj_into_list(obj), writer);
    case GGL_TYPE_MAP:
        return json_write_map(ggl_obj_into_map(obj), writer);
    }
    assert(false);
    return GGL_ERR_FAILURE;
}

GglError ggl_json_encode(GglObject obj, GglWriter writer) {
    return json_write(obj, writer);
}

static GglError obj_read(void *ctx, GglBuffer *buf) {
    assert(buf != NULL);

    GglObject *obj = ctx;

    if ((obj == NULL) || (buf == NULL)) {
        return GGL_ERR_INVALID;
    }

    return ggl_json_encode(*obj, ggl_buf_writer(buf));
}

GglReader ggl_json_reader(GglObject *obj) {
    assert(obj != NULL);
    return (GglReader) { .read = obj_read, .ctx = obj };
}
