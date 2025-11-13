// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/eventstream/decode.h>
#include <gg/eventstream/rpc.h>
#include <gg/eventstream/types.h>
#include <gg/io.h>
#include <gg/log.h>
#include <stdint.h>

GgError eventsteam_get_packet(
    GgReader input, EventStreamMessage msg[static 1], GgBuffer buffer
) {
    GgBuffer prelude_buf = gg_buffer_substr(buffer, 0, 12);
    assert(prelude_buf.len == 12);

    GgError ret = gg_reader_call_exact(input, prelude_buf);
    if (ret != GG_ERR_OK) {
        return ret;
    }

    EventStreamPrelude prelude;
    ret = eventstream_decode_prelude(prelude_buf, &prelude);
    if (ret != GG_ERR_OK) {
        return ret;
    }

    if (prelude.data_len > buffer.len) {
        GG_LOGE("EventStream packet does not fit in IPC packet buffer size.");
        return GG_ERR_NOMEM;
    }

    GgBuffer data_section = gg_buffer_substr(buffer, 0, prelude.data_len);

    ret = gg_reader_call_exact(input, data_section);
    if (ret != GG_ERR_OK) {
        return ret;
    }

    ret = eventstream_decode(&prelude, data_section, msg);
    if (ret != GG_ERR_OK) {
        return ret;
    }

    return GG_ERR_OK;
}

GgError eventstream_get_common_headers(
    EventStreamMessage *msg, EventStreamCommonHeaders *out
) {
    int32_t message_type = 0;
    int32_t message_flags = 0;
    int32_t stream_id = 0;

    EventStreamHeaderIter iter = msg->headers;
    EventStreamHeader header;

    while (eventstream_header_next(&iter, &header) == GG_ERR_OK) {
        if (gg_buffer_eq(header.name, GG_STR(":message-type"))) {
            if (header.value.type != EVENTSTREAM_INT32) {
                GG_LOGE(":message-type header not Int32.");
                return GG_ERR_INVALID;
            }

            message_type = header.value.int32;
        } else if (gg_buffer_eq(header.name, GG_STR(":message-flags"))) {
            if (header.value.type != EVENTSTREAM_INT32) {
                GG_LOGE(":message-flags header not Int32.");
                return GG_ERR_INVALID;
            }

            message_flags = header.value.int32;
        } else if (gg_buffer_eq(header.name, GG_STR(":stream-id"))) {
            if (header.value.type != EVENTSTREAM_INT32) {
                GG_LOGE(":stream-id header not Int32.");
                return GG_ERR_INVALID;
            }

            stream_id = header.value.int32;
        }
    }

    out->message_type = message_type;
    out->message_flags = message_flags;
    out->stream_id = stream_id;

    return GG_ERR_OK;
}
