// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "../crc32.h"
#include <assert.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/eventstream/decode.h>
#include <gg/eventstream/types.h>
#include <gg/log.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

static uint32_t read_be_uint32(GgBuffer buf) {
    assert(buf.len == 4);
    return ((uint32_t) buf.data[0] << 24) | ((uint32_t) buf.data[1] << 16)
        | ((uint32_t) buf.data[2] << 8) | ((uint32_t) buf.data[3]);
}

static int32_t read_be_int32(GgBuffer buf) {
    assert(buf.len == 4);
    uint32_t bytes = read_be_uint32(buf);
    int32_t ret;
    // memcpy necessary for well-defined type-punning
    memcpy(&ret, &bytes, sizeof(ret));
    return ret;
}

GgError eventstream_decode_prelude(GgBuffer buf, EventStreamPrelude *prelude) {
    if (buf.len < 12) {
        return GG_ERR_RANGE;
    }

    uint32_t crc = gg_update_crc(0, gg_buffer_substr(buf, 0, 8));

    uint32_t prelude_crc = read_be_uint32(gg_buffer_substr(buf, 8, 12));

    if (crc != prelude_crc) {
        GG_LOGE("Prelude CRC mismatch.");
        return GG_ERR_PARSE;
    }

    uint32_t message_len = read_be_uint32(gg_buffer_substr(buf, 0, 4));
    uint32_t headers_len = read_be_uint32(gg_buffer_substr(buf, 4, 8));

    // message must at least have 12 byte prelude and 4 byte message crc
    if (message_len < 16) {
        GG_LOGE("Prelude's message length below valid range.");
        return GG_ERR_PARSE;
    }

    if (headers_len > message_len - 16) {
        GG_LOGE("Prelude's header length does not fit in valid range.");
        return GG_ERR_PARSE;
    }

    *prelude = (EventStreamPrelude) {
        .data_len = message_len - 12,
        .headers_len = headers_len,
        .crc = gg_update_crc(prelude_crc, gg_buffer_substr(buf, 8, 12)),
    };
    return GG_ERR_OK;
}

/// Removes next header from buffer
static GgError take_header(GgBuffer *headers_buf) {
    assert(headers_buf != NULL);

    uint32_t pos = 0;

    if ((headers_buf->len - pos) < 1) {
        GG_LOGE("Header parsing out of bounds.");
        return GG_ERR_PARSE;
    }
    uint8_t header_name_len = headers_buf->data[pos];
    pos += 1;

    if ((headers_buf->len - pos) < header_name_len) {
        GG_LOGE("Header parsing out of bounds.");
        return GG_ERR_PARSE;
    }
    pos += header_name_len;

    if ((headers_buf->len - pos) < 1) {
        GG_LOGE("Header parsing out of bounds.");
        return GG_ERR_PARSE;
    }
    uint8_t header_value_type = headers_buf->data[pos];
    pos += 1;

    switch (header_value_type) {
    case EVENTSTREAM_INT32:
        if (headers_buf->len - pos < 4) {
            GG_LOGE("Header parsing out of bounds.");
            return GG_ERR_PARSE;
        }
        pos += 4;
        break;
    case EVENTSTREAM_STRING:
        if (headers_buf->len - pos < 2) {
            GG_LOGE("Header parsing out of bounds.");
            return GG_ERR_PARSE;
        }
        uint16_t value_len = (uint16_t) (headers_buf->data[pos] << 8)
            + headers_buf->data[pos + 1];
        pos += 2;

        if (headers_buf->len - pos < value_len) {
            GG_LOGE("Header parsing out of bounds.");
            return GG_ERR_PARSE;
        }
        pos += value_len;
        break;
    default:
        GG_LOGE("Unsupported header value type.");
        return GG_ERR_PARSE;
    }

    *headers_buf = gg_buffer_substr(*headers_buf, pos, SIZE_MAX);
    return GG_ERR_OK;
}

static GgError count_headers(GgBuffer headers_buf, uint32_t *count) {
    assert(count != NULL);

    uint32_t headers_count = 0;
    GgBuffer headers = headers_buf;

    while (headers.len > 0) {
        GgError err = take_header(&headers);
        if (err != GG_ERR_OK) {
            return err;
        }
        headers_count += 1;
    }

    *count = headers_count;
    return GG_ERR_OK;
}

GgError eventstream_decode(
    const EventStreamPrelude *prelude,
    GgBuffer data_section,
    EventStreamMessage *msg
) {
    assert(msg != NULL);
    assert(data_section.len >= 4);

    GG_LOGT("Decoding eventstream message.");

    uint32_t crc = gg_update_crc(
        prelude->crc, gg_buffer_substr(data_section, 0, data_section.len - 4)
    );

    uint32_t message_crc = read_be_uint32(
        gg_buffer_substr(data_section, data_section.len - 4, data_section.len)
    );

    if (crc != message_crc) {
        GG_LOGE("Message CRC mismatch %u %u.", crc, message_crc);
        return GG_ERR_PARSE;
    }

    GgBuffer headers_buf
        = gg_buffer_substr(data_section, 0, prelude->headers_len);
    GgBuffer payload = gg_buffer_substr(
        data_section, prelude->headers_len, data_section.len - 4
    );

    assert(headers_buf.len == prelude->headers_len);

    uint32_t headers_count = 0;
    GgError err = count_headers(headers_buf, &headers_count);
    if (err != GG_ERR_OK) {
        return err;
    }

    EventStreamHeaderIter header_iter = {
        .pos = headers_buf.data,
        .count = headers_count,
    };

    *msg = (EventStreamMessage) {
        .headers = header_iter,
        .payload = payload,
    };

    // Print out headers at trace level
    EventStreamHeader header;
    while (eventstream_header_next(&header_iter, &header) == GG_ERR_OK) {
        switch (header.value.type) {
        case EVENTSTREAM_INT32:
            GG_LOGT(
                "Header: \"%.*s\" => %d",
                (int) header.name.len,
                header.name.data,
                header.value.int32
            );
            break;
        case EVENTSTREAM_STRING:
            GG_LOGT(
                "Header: \"%.*s\" => (data not shown)",
                (int) header.name.len,
                header.name.data
            );
            break;
        }
    }

    GG_LOGT("Successfully decoded eventstream message.");

    return GG_ERR_OK;
}

GgError eventstream_header_next(
    EventStreamHeaderIter *headers, EventStreamHeader *header
) {
    assert(headers != NULL);
    assert(header != NULL);

    if (headers->count < 1) {
        return GG_ERR_RANGE;
    }

    uint8_t *pos = headers->pos;
    uint8_t header_name_len = pos[0];
    pos += 1;

    GgBuffer header_name = {
        .data = pos,
        .len = header_name_len,
    };

    pos += header_name_len;

    uint8_t header_value_type = pos[0];
    pos += 1;

    EventStreamHeaderValue value;

    switch (header_value_type) {
    case EVENTSTREAM_INT32:
        value.type = EVENTSTREAM_INT32;
        value.int32 = read_be_int32((GgBuffer) { .data = pos, .len = 4 });
        pos += 4;
        break;
    case EVENTSTREAM_STRING: {
        value.type = EVENTSTREAM_STRING;
        uint16_t str_len = (uint16_t) (pos[0] << 8) + pos[1];
        pos += 2;
        value.string = (GgBuffer) { .data = pos, .len = str_len };
        pos += str_len;
        break;
    }
    default:
        assert(false);
        return GG_ERR_PARSE;
    }

    headers->pos = pos;
    headers->count -= 1;

    *header = (EventStreamHeader) {
        .name = header_name,
        .value = value,
    };

    return GG_ERR_OK;
}
