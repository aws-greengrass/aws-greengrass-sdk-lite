// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <float.h>
#include <ggl/buffer.h>
#include <ggl/constants.h>
#include <ggl/error.h>
#include <ggl/io.h>
#include <ggl/json_encode.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <ggl/vector.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
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
    char encoded[DBL_DECIMAL_DIG + 9]; // -x.<precision>E-xxx\0
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static GglError json_write(GglObject obj, GglWriter writer) {
    typedef struct {
        enum {
            LEVEL_DEFAULT,
            LEVEL_LIST_START,
            LEVEL_LIST,
            LEVEL_MAP_START,
            LEVEL_MAP,
        } level_type;

        uint8_t remaining;

        union {
            GglObject *obj_next;
            GglKV *kv_next;
        };
    } IterLevel;

    IterLevel iter_state[GGL_MAX_OBJECT_DEPTH] = { {
        .level_type = LEVEL_DEFAULT,
        .remaining = 1,
        .obj_next = &obj,
    } };
    size_t state_level = 1;

    do {
        IterLevel *level = &iter_state[state_level - 1];
        GglError ret;

        if (level->remaining == 0) {
            switch (level->level_type) {
            case LEVEL_DEFAULT:
                break;
            case LEVEL_LIST_START:
            case LEVEL_LIST:
                ret = ggl_writer_call(writer, GGL_STR("]"));
                if (ret != GGL_ERR_OK) {
                    return ret;
                }
                break;
            case LEVEL_MAP_START:
            case LEVEL_MAP:
                ret = ggl_writer_call(writer, GGL_STR("}"));
                if (ret != GGL_ERR_OK) {
                    return ret;
                }
                break;
            }
            state_level -= 1;
            continue;
        }

        switch (level->level_type) {
        case LEVEL_LIST:
        case LEVEL_MAP:
            ret = ggl_writer_call(writer, GGL_STR(","));
            if (ret != GGL_ERR_OK) {
                return ret;
            }
            break;
        case LEVEL_LIST_START:
            level->level_type = LEVEL_LIST;
            break;
        case LEVEL_MAP_START:
            level->level_type = LEVEL_MAP;
            break;
        case LEVEL_DEFAULT:;
        }

        GglObject *cur_obj;
        if (level->level_type == LEVEL_MAP) {
            ret = json_write_buf(ggl_kv_key(*level->kv_next), writer);
            if (ret != GGL_ERR_OK) {
                return ret;
            }
            ret = ggl_writer_call(writer, GGL_STR(":"));
            if (ret != GGL_ERR_OK) {
                return ret;
            }

            cur_obj = ggl_kv_val(level->kv_next);
            level->kv_next = &level->kv_next[1];
        } else {
            cur_obj = level->obj_next;
            level->obj_next = &level->obj_next[1];
        }

        level->remaining -= 1;

        switch (ggl_obj_type(*cur_obj)) {
        case GGL_TYPE_NULL:
            ret = json_write_null(writer);
            if (ret != GGL_ERR_OK) {
                return ret;
            }
            break;
        case GGL_TYPE_BOOLEAN:
            ret = json_write_bool(ggl_obj_into_bool(*cur_obj), writer);
            if (ret != GGL_ERR_OK) {
                return ret;
            }
            break;
        case GGL_TYPE_I64:
            ret = json_write_i64(ggl_obj_into_i64(*cur_obj), writer);
            if (ret != GGL_ERR_OK) {
                return ret;
            }
            break;
        case GGL_TYPE_F64:
            ret = json_write_f64(ggl_obj_into_f64(*cur_obj), writer);
            if (ret != GGL_ERR_OK) {
                return ret;
            }
            break;
        case GGL_TYPE_BUF:
            ret = json_write_buf(ggl_obj_into_buf(*cur_obj), writer);
            if (ret != GGL_ERR_OK) {
                return ret;
            }
            break;
        case GGL_TYPE_LIST: {
            if (state_level >= GGL_MAX_OBJECT_DEPTH) {
                return GGL_ERR_RANGE;
            }

            ret = ggl_writer_call(writer, GGL_STR("["));
            if (ret != GGL_ERR_OK) {
                return ret;
            }

            GglList list = ggl_obj_into_list(*cur_obj);
            if (list.len > UINT8_MAX) {
                return GGL_ERR_RANGE;
            }
            iter_state[state_level] = (IterLevel) {
                .level_type = LEVEL_LIST_START,
                .remaining = (uint8_t) list.len,
                .obj_next = list.items,
            };
            state_level += 1;
        } break;
        case GGL_TYPE_MAP: {
            if (state_level >= GGL_MAX_OBJECT_DEPTH) {
                return GGL_ERR_RANGE;
            }

            ret = ggl_writer_call(writer, GGL_STR("{"));
            if (ret != GGL_ERR_OK) {
                return ret;
            }

            GglMap map = ggl_obj_into_map(*cur_obj);
            if (map.len > UINT8_MAX) {
                return GGL_ERR_RANGE;
            }
            iter_state[state_level] = (IterLevel) {
                .level_type = LEVEL_MAP_START,
                .remaining = (uint8_t) map.len,
                .kv_next = map.pairs,
            };
            state_level += 1;
        } break;
        }
    } while (state_level > 0);

    return GGL_ERR_OK;
}

GglError ggl_json_encode(GglObject obj, GglWriter writer) {
    return json_write(obj, writer);
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
