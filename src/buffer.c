// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <gg/buffer.h>
#include <gg/cbmc.h>
#include <gg/error.h>
#include <gg/io.h>
#include <gg/log.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

GgBuffer gg_buffer_from_null_term(char str[static 1]) {
    assert(str != NULL);
    return (GgBuffer) { .data = (uint8_t *) str, .len = strlen(str) };
}

bool gg_buffer_eq(GgBuffer buf1, GgBuffer buf2) {
    if (buf1.len == buf2.len) {
        if (buf1.len == 0) {
            return true;
        }
        return memcmp(buf1.data, buf2.data, buf1.len) == 0;
    }
    return false;
}

bool gg_buffer_has_prefix(GgBuffer buf, GgBuffer prefix) {
    if (prefix.len <= buf.len) {
        return memcmp(buf.data, prefix.data, prefix.len) == 0;
    }
    return false;
}

bool gg_buffer_remove_prefix(GgBuffer buf[static 1], GgBuffer prefix) {
    assert(buf != NULL);
    if (gg_buffer_has_prefix(*buf, prefix)) {
        *buf = gg_buffer_substr(*buf, prefix.len, SIZE_MAX);
        return true;
    }
    return false;
}

bool gg_buffer_has_suffix(GgBuffer buf, GgBuffer suffix) {
    if (suffix.len <= buf.len) {
        return memcmp(&buf.data[buf.len - suffix.len], suffix.data, suffix.len)
            == 0;
    }
    return false;
}

bool gg_buffer_remove_suffix(GgBuffer buf[static 1], GgBuffer suffix) {
    assert(buf != NULL);
    if (gg_buffer_has_suffix(*buf, suffix)) {
        *buf = gg_buffer_substr(*buf, 0, buf->len - suffix.len);
        return true;
    }
    return false;
}

bool gg_buffer_contains(GgBuffer buf, GgBuffer substring, size_t *start) {
    if (substring.len == 0) {
        if (start != NULL) {
            *start = 0;
        }
        return true;
    }
    uint8_t *ptr = memmem(buf.data, buf.len, substring.data, substring.len);
    if (ptr == NULL) {
        return false;
    }
    if (start != NULL) {
        *start = (size_t) (ptr - buf.data);
    }
    return true;
}

GgBuffer gg_buffer_substr(GgBuffer buf, size_t start, size_t end) {
    size_t start_trunc = start < buf.len ? start : buf.len;
    size_t end_trunc = end < buf.len ? end : buf.len;
    return (GgBuffer) {
        .data = &buf.data[start_trunc],
        .len = end_trunc >= start_trunc ? end_trunc - start_trunc : 0U,
    };
}

static bool mult_overflow_int64(int64_t a, int64_t b) {
    if (b == 0) {
        return false;
    }
    return b > 0 ? ((a > INT64_MAX / b) || (a < INT64_MIN / b))
                 : ((a < INT64_MAX / b) || (a > INT64_MIN / b));
}

static bool add_overflow_int64(int64_t a, int64_t b) {
    return b > 0 ? (a > INT64_MAX - b) : (a < INT64_MIN - b);
}

GgError gg_str_to_int64(GgBuffer str, int64_t value[static 1]) {
    int64_t ret = 0;
    int64_t sign = 1;
    size_t i = 0;

    if ((str.len >= 1) && (str.data[0] == '-')) {
        i = 1;
        sign = -1;
    }

    if (i == str.len) {
        GG_LOGE("Insufficient characters when parsing int64.");
        return GG_ERR_INVALID;
    }

    for (; i < str.len; i++)
        CBMC_CONTRACT(assigns(ret, i), decreases(str.len - i)) {
            uint8_t c = str.data[i];

            if ((c < '0') || (c > '9')) {
                GG_LOGE("Invalid character %c when parsing int64.", c);
                return GG_ERR_INVALID;
            }

            if (mult_overflow_int64(ret, 10U)) {
                GG_LOGE("Overflow when parsing int64 from buffer.");
                return GG_ERR_RANGE;
            }
            ret *= 10;

            if (add_overflow_int64(ret, sign * (c - '0'))) {
                GG_LOGE("Overflow when parsing int64 from buffer.");
                return GG_ERR_RANGE;
            }
            ret += sign * (c - '0');
        }

    *value = ret;
    return GG_ERR_OK;
}

static GgError buf_write(void *ctx, GgBuffer buf) {
    GgBuffer *target = ctx;
    GgBuffer target_copy = *target;

    GgError ret = gg_buf_copy(buf, &target_copy);
    if (ret != GG_ERR_OK) {
        return ret;
    }

    *target = gg_buffer_substr(*target, buf.len, SIZE_MAX);
    return GG_ERR_OK;
}

GgWriter gg_buf_writer(GgBuffer *buf) {
    return (GgWriter) { .ctx = buf, .write = &buf_write };
}

GgError gg_buf_copy(GgBuffer source, GgBuffer *target) {
    if (source.len == 0) {
        target->len = 0;
        GG_LOGT("Source has zero length buffer");
        return GG_ERR_OK;
    }
    if (target->len < source.len) {
        GG_LOGT("Failed to copy buffer due to insufficient space.");
        return GG_ERR_NOMEM;
    }
    memcpy(target->data, source.data, source.len);
    target->len = source.len;

    return GG_ERR_OK;
}
