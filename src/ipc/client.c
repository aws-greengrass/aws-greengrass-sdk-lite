// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <ggl/arena.h>
#include <ggl/buffer.h>
#include <ggl/cleanup.h>
#include <ggl/error.h>
#include <ggl/eventstream/decode.h>
#include <ggl/eventstream/encode.h>
#include <ggl/eventstream/rpc.h>
#include <ggl/eventstream/types.h>
#include <ggl/file.h> // IWYU pragma: keep (TODO: remove after file.h refactor)
#include <ggl/flags.h>
#include <ggl/io.h>
#include <ggl/ipc/client.h>
#include <ggl/ipc/error.h>
#include <ggl/json_decode.h>
#include <ggl/json_encode.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <ggl/socket.h>
#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>

// Maximum size of eventstream packet.
/// Can be configured with `-D GGL_IPC_MAX_MSG_LEN=<N>`.
#ifndef GGL_IPC_MAX_MSG_LEN
#define GGL_IPC_MAX_MSG_LEN 10000
#endif

static uint8_t payload_array[GGL_IPC_MAX_MSG_LEN];
static pthread_mutex_t payload_array_mtx = PTHREAD_MUTEX_INITIALIZER;

static GglError send_message(
    int conn,
    const EventStreamHeader *headers,
    size_t headers_len,
    GglMap *payload
) {
    GGL_MTX_SCOPE_GUARD(&payload_array_mtx);

    GglBuffer send_buffer = GGL_BUF(payload_array);

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

static GglError get_message(
    int conn,
    GglBuffer recv_buffer,
    EventStreamMessage *msg,
    EventStreamCommonHeaders *common_headers
) {
    assert(msg != NULL);

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

    if (common_headers != NULL) {
        ret = eventstream_get_common_headers(msg, common_headers);
        if (ret != GGL_ERR_OK) {
            return ret;
        }
    }

    return GGL_ERR_OK;
}

GglError ggipc_connect_by_name(
    GglBuffer socket_path, GglBuffer component_name, GglBuffer *svcuid, int *fd
) {
    int conn = -1;
    GglError ret = ggl_connect(socket_path, &conn);
    if (ret != GGL_ERR_OK) {
        return ret;
    }
    GGL_CLEANUP_ID(conn_cleanup, cleanup_close, conn);

    EventStreamHeader headers[] = {
        { GGL_STR(":message-type"),
          { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_CONNECT } },
        { GGL_STR(":message-flags"), { EVENTSTREAM_INT32, .int32 = 0 } },
        { GGL_STR(":stream-id"), { EVENTSTREAM_INT32, .int32 = 0 } },
        { GGL_STR(":version"),
          { EVENTSTREAM_STRING, .string = GGL_STR("0.1.0") } },
    };
    size_t headers_len = sizeof(headers) / sizeof(headers[0]);

    GglMap payload
        = GGL_MAP(ggl_kv(GGL_STR("componentName"), ggl_obj_buf(component_name))
        );

    ret = send_message(conn, headers, headers_len, &payload);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    GGL_MTX_SCOPE_GUARD(&payload_array_mtx);

    GglBuffer recv_buffer = GGL_BUF(payload_array);
    EventStreamMessage msg = { 0 };
    EventStreamCommonHeaders common_headers;

    ret = get_message(conn, recv_buffer, &msg, &common_headers);

    if (ret != GGL_ERR_OK) {
        return ret;
    }

    if (common_headers.message_type != EVENTSTREAM_CONNECT_ACK) {
        GGL_LOGE("Connection response not an ack.");
        return GGL_ERR_FAILURE;
    }

    if ((common_headers.message_flags & EVENTSTREAM_CONNECTION_ACCEPTED) == 0) {
        GGL_LOGE("Connection response missing accepted flag.");
        return GGL_ERR_FAILURE;
    }

    if (msg.payload.len != 0) {
        GGL_LOGW("Eventstream connection ack has unexpected payload.");
    }

    EventStreamHeaderIter iter = msg.headers;
    EventStreamHeader header;

    while (eventstream_header_next(&iter, &header) == GGL_ERR_OK) {
        if (ggl_buffer_eq(header.name, GGL_STR("svcuid"))) {
            if (header.value.type != EVENTSTREAM_STRING) {
                GGL_LOGE("Response svcuid header not string.");
                return GGL_ERR_INVALID;
            }

            if (svcuid != NULL) {
                if (svcuid->len < header.value.string.len) {
                    GGL_LOGE("Insufficient buffer space for svcuid.");
                    return GGL_ERR_NOMEM;
                }

                memcpy(
                    svcuid->data,
                    header.value.string.data,
                    header.value.string.len
                );
                svcuid->len = header.value.string.len;
            }

            if (fd != NULL) {
                conn_cleanup = -1;
                *fd = conn;
            }

            return GGL_ERR_OK;
        }
    }

    GGL_LOGE("Response missing svcuid header.");
    return GGL_ERR_FAILURE;
}

GglError ggipc_call(
    int conn,
    GglBuffer operation,
    GglBuffer service_model_type,
    GglMap params,
    GglArena *alloc,
    GglObject *result,
    GglIpcError *remote_err
) {
    EventStreamHeader headers[] = {
        { GGL_STR(":message-type"),
          { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_APPLICATION_MESSAGE } },
        { GGL_STR(":message-flags"), { EVENTSTREAM_INT32, .int32 = 0 } },
        { GGL_STR(":stream-id"), { EVENTSTREAM_INT32, .int32 = 1 } },
        { GGL_STR("operation"), { EVENTSTREAM_STRING, .string = operation } },
        { GGL_STR("service-model-type"),
          { EVENTSTREAM_STRING, .string = service_model_type } },
    };
    size_t headers_len = sizeof(headers) / sizeof(headers[0]);

    GglError ret = send_message(conn, headers, headers_len, &params);
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to send message %d", ret);
        return ret;
    }

    GGL_MTX_SCOPE_GUARD(&payload_array_mtx);

    GglBuffer recv_buffer = GGL_BUF(payload_array);
    EventStreamMessage msg = { 0 };
    EventStreamCommonHeaders common_headers;

    ret = get_message(conn, recv_buffer, &msg, &common_headers);
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("get_message returned %d", ret);
        return ret;
    }

    if (common_headers.stream_id != 1) {
        GGL_LOGE("Unknown stream id received.");
        return GGL_ERR_FAILURE;
    }

    if (common_headers.message_type == EVENTSTREAM_APPLICATION_ERROR) {
        GGL_LOGE(
            "Received an IPC error on stream %" PRId32 ".",
            common_headers.stream_id
        );

        if (remote_err == NULL) {
            return GGL_ERR_REMOTE;
        }

        GglArena error_alloc
            = ggl_arena_init(GGL_BUF((uint8_t[sizeof(GglObject) * 4]) { 0 }));

        GglObject err_result;
        ret = ggl_json_decode_destructive(
            msg.payload, &error_alloc, &err_result
        );
        if (ret != GGL_ERR_OK) {
            GGL_LOGE("Failed to decode IPC error payload.");
            return ret;
        }
        if (ggl_obj_type(err_result) != GGL_TYPE_MAP) {
            GGL_LOGE("Failed to decode IPC error payload.");
            return GGL_ERR_PARSE;
        }

        GglObject *error_code_obj;
        GglObject *message_obj;

        ret = ggl_map_validate(
            ggl_obj_into_map(err_result),
            GGL_MAP_SCHEMA(
                { GGL_STR("_errorCode"),
                  GGL_REQUIRED,
                  GGL_TYPE_BUF,
                  &error_code_obj },
                { GGL_STR("_message"),
                  GGL_OPTIONAL,
                  GGL_TYPE_BUF,
                  &message_obj },
            )
        );
        if (ret != GGL_ERR_OK) {
            GGL_LOGE("Error response does not match known schema.");
            return ret;
        }
        GglBuffer error_code = ggl_obj_into_buf(*error_code_obj);

        remote_err->error_code = get_ipc_err_info(error_code);
        remote_err->message = GGL_STR("");

        if (message_obj != NULL) {
            GglBuffer err_msg = ggl_obj_into_buf(*message_obj);

            ret = ggl_arena_claim_buf(&err_msg, alloc);
            if (ret != GGL_ERR_OK) {
                GGL_LOGW("Insufficient memory provided for IPC error message.");
                // TODO: truncate error message to what fits
            } else {
                remote_err->message = err_msg;
            }
        }

        return GGL_ERR_REMOTE;
    }

    if (common_headers.message_type != EVENTSTREAM_APPLICATION_MESSAGE) {
        GGL_LOGE(
            "Unexpected message type %" PRId32 ".", common_headers.message_type
        );
        return GGL_ERR_FAILURE;
    }

    if (result != NULL) {
        ret = ggl_json_decode_destructive(msg.payload, alloc, result);
        if (ret != GGL_ERR_OK) {
            GGL_LOGE("Failed to decode IPC response payload.");
            return ret;
        }

        ret = ggl_arena_claim_obj(result, alloc);
        if (ret != GGL_ERR_OK) {
            GGL_LOGE("Insufficient memory provided for IPC response payload.");
            return ret;
        }
    }

    return GGL_ERR_OK;
}
