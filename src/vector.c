// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/io.h>
#include <gg/list.h>
#include <gg/log.h>
#include <gg/object.h>
#include <gg/vector.h>
#include <string.h>
#include <stdint.h>

GgError gg_obj_vec_push(GgObjVec *vector, GgObject object) {
    if (vector->list.len >= vector->capacity) {
        return GG_ERR_NOMEM;
    }
    GG_LOGT("Pushed to %p.", vector);
    vector->list.items[vector->list.len] = object;
    vector->list.len++;
    return GG_ERR_OK;
}

void gg_obj_vec_chain_push(GgError *err, GgObjVec *vector, GgObject object) {
    if (*err == GG_ERR_OK) {
        *err = gg_obj_vec_push(vector, object);
    }
}

GgError gg_obj_vec_pop(GgObjVec *vector, GgObject *out) {
    if (vector->list.len == 0) {
        return GG_ERR_RANGE;
    }
    if (out != NULL) {
        *out = vector->list.items[vector->list.len - 1];
    }
    GG_LOGT("Popped from %p.", vector);

    vector->list.len--;
    return GG_ERR_OK;
}

GgError gg_obj_vec_append(GgObjVec *vector, GgList list) {
    if (vector->capacity - vector->list.len < list.len) {
        return GG_ERR_NOMEM;
    }
    GG_LOGT("Appended to %p.", vector);
    if (list.len > 0) {
        memcpy(
            &vector->list.items[vector->list.len],
            list.items,
            list.len * sizeof(GgObject)
        );
    }
    vector->list.len += list.len;
    return GG_ERR_OK;
}

void gg_obj_vec_chain_append(GgError *err, GgObjVec *vector, GgList list) {
    if (*err == GG_ERR_OK) {
        *err = gg_obj_vec_append(vector, list);
    }
}

GgError gg_kv_vec_push(GgKVVec *vector, GgKV kv) {
    if (vector->map.len >= vector->capacity) {
        return GG_ERR_NOMEM;
    }
    GG_LOGT("Pushed to %p.", vector);
    vector->map.pairs[vector->map.len] = kv;
    vector->map.len++;
    return GG_ERR_OK;
}

GgByteVec gg_byte_vec_init(GgBuffer buf) {
    return (GgByteVec) { .buf = { .data = buf.data, .len = 0 },
                         .capacity = buf.len };
}

GgError gg_byte_vec_push(GgByteVec *vector, uint8_t byte) {
    if (vector->buf.len >= vector->capacity) {
        return GG_ERR_NOMEM;
    }
    GG_LOGT("Pushed to %p.", vector);
    vector->buf.data[vector->buf.len] = byte;
    vector->buf.len++;
    return GG_ERR_OK;
}

void gg_byte_vec_chain_push(GgError *err, GgByteVec *vector, uint8_t byte) {
    if (*err == GG_ERR_OK) {
        *err = gg_byte_vec_push(vector, byte);
    }
}

GgError gg_byte_vec_append(GgByteVec *vector, GgBuffer buf) {
    if (vector->capacity - vector->buf.len < buf.len) {
        return GG_ERR_NOMEM;
    }
    GG_LOGT("Appended to %p.", vector);
    if (buf.len > 0) {
        memcpy(&vector->buf.data[vector->buf.len], buf.data, buf.len);
    }
    vector->buf.len += buf.len;
    return GG_ERR_OK;
}

void gg_byte_vec_chain_append(GgError *err, GgByteVec *vector, GgBuffer buf) {
    if (*err == GG_ERR_OK) {
        *err = gg_byte_vec_append(vector, buf);
    }
}

GgBuffer gg_byte_vec_remaining_capacity(GgByteVec vector) {
    return (GgBuffer) { .data = &vector.buf.data[vector.buf.len],
                        .len = vector.capacity - vector.buf.len };
}

static GgError byte_vec_write(void *ctx, GgBuffer buf) {
    GgByteVec *target = ctx;
    return gg_byte_vec_append(target, buf);
}

GgWriter gg_byte_vec_writer(GgByteVec *byte_vec) {
    return (GgWriter) { .ctx = byte_vec, .write = &byte_vec_write };
}

GgError gg_buf_vec_push(GgBufVec *vector, GgBuffer buf) {
    if (vector->buf_list.len >= vector->capacity) {
        return GG_ERR_NOMEM;
    }
    GG_LOGT("Pushed to %p.", vector);
    vector->buf_list.bufs[vector->buf_list.len] = buf;
    vector->buf_list.len++;
    return GG_ERR_OK;
}

void gg_buf_vec_chain_push(GgError *err, GgBufVec *vector, GgBuffer buf) {
    if (*err == GG_ERR_OK) {
        *err = gg_buf_vec_push(vector, buf);
    }
}

GgError gg_buf_vec_append_list(GgBufVec *vector, GgList list) {
    GG_LIST_FOREACH(item, list) {
        if (gg_obj_type(*item) != GG_TYPE_BUF) {
            return GG_ERR_INVALID;
        }
        GgError ret = gg_buf_vec_push(vector, gg_obj_into_buf(*item));
        if (ret != GG_ERR_OK) {
            return ret;
        }
    }
    return GG_ERR_OK;
}

void gg_buf_vec_chain_append_list(GgError *err, GgBufVec *vector, GgList list) {
    if (*err == GG_ERR_OK) {
        *err = gg_buf_vec_append_list(vector, list);
    }
}
