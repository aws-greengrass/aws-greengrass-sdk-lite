// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "eventstream_io.h"
#include <assert.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/eventstream/decode.h>
#include <ggl/eventstream/encode.h>
#include <ggl/eventstream/types.h>
#include <ggl/file.h> // IWYU pragma: keep (TODO: remove after file.h refactor)
#include <ggl/io.h>
#include <ggl/json_encode.h>
#include <ggl/log.h>
#include <ggl/object.h>
#include <ggl/socket.h>

GglError ggipc_send_message(
    int conn,
    const EventStreamHeader *headers,
    size_t headers_len,
    GglMap *payload,
    GglBuffer send_buffer
) {
    GglObject payload_obj;
    GglReader reader;
    if (payload == NULL) {
        reader = GGL_NULL_READER;
    } else {
        payload_obj = ggl_obj_map(*payload);
        reader = ggl_json_reader(&payload_obj);
    }
    GglError ret
        = eventstream_encode(&send_buffer, headers, headers_len, reader);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    return ggl_socket_write(conn, send_buffer);
}

GglError ggipc_get_message(
    int conn, EventStreamMessage *msg, GglBuffer recv_buffer
) {
    GglBuffer prelude_buf = ggl_buffer_substr(recv_buffer, 0, 12);
    assert(prelude_buf.len == 12);

    GglError ret = ggl_socket_read(conn, prelude_buf);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    EventStreamPrelude prelude;
    ret = eventstream_decode_prelude(prelude_buf, &prelude);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    if (prelude.data_len > recv_buffer.len) {
        GGL_LOGE("EventStream packet does not fit in IPC packet buffer size.");
        return GGL_ERR_NOMEM;
    }

    GglBuffer data_section
        = ggl_buffer_substr(recv_buffer, 0, prelude.data_len);

    ret = ggl_socket_read(conn, data_section);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    ret = eventstream_decode(&prelude, data_section, msg);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    return GGL_ERR_OK;
}
