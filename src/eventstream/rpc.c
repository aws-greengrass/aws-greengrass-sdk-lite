// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/eventstream/decode.h>
#include <ggl/eventstream/rpc.h>
#include <ggl/eventstream/types.h>
#include <ggl/io.h>
#include <ggl/log.h>
#include <stdint.h>

GglError eventsteam_get_packet(
    GglReader input, EventStreamMessage msg[static 1], GglBuffer buffer
) {
    GglBuffer prelude_buf = ggl_buffer_substr(buffer, 0, 12);
    assert(prelude_buf.len == 12);

    GglError ret = ggl_reader_call_exact(input, prelude_buf);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    EventStreamPrelude prelude;
    ret = eventstream_decode_prelude(prelude_buf, &prelude);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    if (prelude.data_len > buffer.len) {
        GGL_LOGE("EventStream packet does not fit in IPC packet buffer size.");
        return GGL_ERR_NOMEM;
    }

    GglBuffer data_section = ggl_buffer_substr(buffer, 0, prelude.data_len);

    ret = ggl_reader_call_exact(input, data_section);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    ret = eventstream_decode(&prelude, data_section, msg);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    return GGL_ERR_OK;
}

GglError eventstream_get_common_headers(
    EventStreamMessage *msg, EventStreamCommonHeaders *out
) {
    int32_t message_type = 0;
    int32_t message_flags = 0;
    int32_t stream_id = 0;

    EventStreamHeaderIter iter = msg->headers;
    EventStreamHeader header;

    while (eventstream_header_next(&iter, &header) == GGL_ERR_OK) {
        if (ggl_buffer_eq(header.name, GGL_STR(":message-type"))) {
            if (header.value.type != EVENTSTREAM_INT32) {
                GGL_LOGE(":message-type header not Int32.");
                return GGL_ERR_INVALID;
            }

            message_type = header.value.int32;
        } else if (ggl_buffer_eq(header.name, GGL_STR(":message-flags"))) {
            if (header.value.type != EVENTSTREAM_INT32) {
                GGL_LOGE(":message-flags header not Int32.");
                return GGL_ERR_INVALID;
            }

            message_flags = header.value.int32;
        } else if (ggl_buffer_eq(header.name, GGL_STR(":stream-id"))) {
            if (header.value.type != EVENTSTREAM_INT32) {
                GGL_LOGE(":stream-id header not Int32.");
                return GGL_ERR_INVALID;
            }

            stream_id = header.value.int32;
        }
    }

    out->message_type = message_type;
    out->message_flags = message_flags;
    out->stream_id = stream_id;

    return GGL_ERR_OK;
}
