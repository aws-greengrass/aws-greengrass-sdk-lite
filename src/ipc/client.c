// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "client/eventstream_io.h"
#include "client/receive.h"
#include <assert.h>
#include <errno.h>
#include <ggl/arena.h>
#include <ggl/buffer.h>
#include <ggl/cleanup.h>
#include <ggl/error.h>
#include <ggl/eventstream/decode.h>
#include <ggl/eventstream/rpc.h>
#include <ggl/eventstream/types.h>
#include <ggl/file.h> // IWYU pragma: keep (TODO: remove after file.h refactor)
#include <ggl/flags.h>
#include <ggl/ipc/client.h>
#include <ggl/ipc/client_priv.h>
#include <ggl/ipc/client_raw.h>
#include <ggl/ipc/limits.h>
#include <ggl/json_decode.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <ggl/socket.h>
#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>

/// Protects payload array
/// Serializes requests to IPC server
static pthread_mutex_t call_state_mtx = PTHREAD_MUTEX_INITIALIZER;

static uint8_t payload_array[GGL_IPC_MAX_MSG_LEN];

/// Only used from callbacks on response thread
static uint8_t response_decode_mem[GGL_IPC_PAYLOAD_MAX_SUBOBJECTS];

static int conn_fd = -1;

typedef struct {
    GgIpcSubscribeCallback *fn;
    void *ctx;
} CallerSubHandler;

static CallerSubHandler sub_callbacks[GGL_IPC_MAX_STREAMS - 1];

static bool connected(void) {
    return conn_fd >= 0;
}

static GglError ggipc_connect_with_payload(
    GglBuffer socket_path, GglMap payload, GglBuffer *svcuid
) {
    assert(!connected());

    int conn = -1;
    GglError ret = ggl_connect(socket_path, &conn);
    if (ret != GGL_ERR_OK) {
        GGL_LOGE(
            "Failed to connect to GG-IPC socket at %.*s.",
            (int) socket_path.len,
            socket_path.data
        );
        return ret;
    }
    GGL_CLEANUP_ID(conn_cleanup, cleanup_close, conn);

    GGL_LOGI(
        "Connected to GG-IPC socket at %.*s on fd %d",
        (int) socket_path.len,
        socket_path.data,
        conn
    );

    EventStreamHeader headers[] = {
        { GGL_STR(":message-type"),
          { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_CONNECT } },
        { GGL_STR(":message-flags"), { EVENTSTREAM_INT32, .int32 = 0 } },
        { GGL_STR(":stream-id"), { EVENTSTREAM_INT32, .int32 = 0 } },
        { GGL_STR(":version"),
          { EVENTSTREAM_STRING, .string = GGL_STR("0.1.0") } },
    };
    size_t headers_len = sizeof(headers) / sizeof(headers[0]);

    GGL_MTX_SCOPE_GUARD(&call_state_mtx);

    ret = ggipc_send_message(
        conn, headers, headers_len, &payload, GGL_BUF(payload_array)
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to send GG-IPC connect packet on fd %d.", conn);
        return ret;
    }

    EventStreamMessage msg = { 0 };
    ret = ggipc_get_message(conn, &msg, GGL_BUF(payload_array));
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to receive GG-IPC connect ack on fd %d.", conn);
        return ret;
    }

    EventStreamCommonHeaders common_headers;
    ret = eventstream_get_common_headers(&msg, &common_headers);
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to parse response headers on GG-IPC fd %d.", conn);
        return ret;
    }

    if (common_headers.message_type != EVENTSTREAM_CONNECT_ACK) {
        GGL_LOGE("GG-IPC fd %d connection response not an ack.", conn);
        return GGL_ERR_FAILURE;
    }

    if ((common_headers.message_flags & EVENTSTREAM_CONNECTION_ACCEPTED) == 0) {
        GGL_LOGE(
            "GG-IPC fd %d connection response missing accepted flag.", conn
        );
        return GGL_ERR_FAILURE;
    }

    if (msg.payload.len != 0) {
        GGL_LOGW(
            "GG-IPC fd %d eventstream connection ack has unexpected payload.",
            conn
        );
    }

    ret = ggipc_register_ipc_socket(conn);
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to register GG-IPC fd %d for receiving.", conn);
        return ret;
    }

    if (svcuid == NULL) {
        conn_cleanup = -1;
        conn_fd = conn;

        return GGL_ERR_OK;
    }

    if (svcuid->len < GGL_IPC_SVCUID_STR_LEN) {
        GGL_LOGE("Insufficient buffer space provided for svcuid.");
        return GGL_ERR_NOMEM;
    }

    EventStreamHeaderIter iter = msg.headers;
    EventStreamHeader header;

    while (eventstream_header_next(&iter, &header) == GGL_ERR_OK) {
        if (ggl_buffer_eq(header.name, GGL_STR("svcuid"))) {
            if (header.value.type != EVENTSTREAM_STRING) {
                GGL_LOGE("Response svcuid header not string.");
                return GGL_ERR_INVALID;
            }

            if (svcuid->len < header.value.string.len) {
                GGL_LOGE("Response svcuid too long.");
                return GGL_ERR_NOMEM;
            }

            memcpy(
                svcuid->data, header.value.string.data, header.value.string.len
            );
            svcuid->len = header.value.string.len;

            conn_cleanup = -1;
            conn_fd = conn;

            return GGL_ERR_OK;
        }
    }

    GGL_LOGE("Response missing svcuid header.");
    return GGL_ERR_FAILURE;
}

GglError ggipc_connect_with_name(
    GglBuffer socket_path, GglBuffer component_name, GglBuffer *svcuid
) {
    return ggipc_connect_with_payload(
        socket_path,
        GGL_MAP(ggl_kv(GGL_STR("componentName"), ggl_obj_buf(component_name))),
        svcuid
    );
}

GglError ggipc_connect_with_token(GglBuffer socket_path, GglBuffer auth_token) {
    return ggipc_connect_with_payload(
        socket_path,
        GGL_MAP(ggl_kv(GGL_STR("authToken"), ggl_obj_buf(auth_token))),
        NULL
    );
}

GglError ggipc_connect(void) {
    // Unsafe, but function is documented as such
    // NOLINTBEGIN(concurrency-mt-unsafe)
    char *svcuid = getenv("SVCUID");
    if (svcuid == NULL) {
        return GGL_ERR_CONFIG;
    }
    char *socket_path
        = getenv("AWS_GG_NUCLEUS_DOMAIN_SOCKET_FILEPATH_FOR_COMPONENT");
    if (socket_path == NULL) {
        return GGL_ERR_CONFIG;
    }

    return ggipc_connect_with_token(
        ggl_buffer_from_null_term(socket_path),
        ggl_buffer_from_null_term(svcuid)
    );
    // NOLINTEND(concurrency-mt-unsafe)
}

static GglError handle_application_error(
    GglBuffer payload, GgIpcErrorCallback *error_callback, void *response_ctx
) {
    if (error_callback == NULL) {
        return GGL_ERR_REMOTE;
    }

    GglArena error_alloc = ggl_arena_init(GGL_BUF(response_decode_mem));

    GglObject err_result;
    GglError ret
        = ggl_json_decode_destructive(payload, &error_alloc, &err_result);
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
            { GGL_STR("_message"), GGL_OPTIONAL, GGL_TYPE_BUF, &message_obj },
        )
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Error response does not match known schema.");
        return ret;
    }
    GglBuffer error_code = ggl_obj_into_buf(*error_code_obj);

    GglBuffer message = GGL_STR("");
    if (message_obj != NULL) {
        message = ggl_obj_into_buf(*message_obj);
    }

    ret = error_callback(response_ctx, error_code, message);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    return GGL_ERR_REMOTE;
}

static GglError decode_response(
    EventStreamCommonHeaders common_headers,
    EventStreamMessage msg,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_ctx
) {
    if (common_headers.message_type == EVENTSTREAM_APPLICATION_ERROR) {
        GGL_LOGD(
            "Received an IPC error on stream %" PRId32 ".",
            common_headers.stream_id
        );

        return handle_application_error(
            msg.payload, error_callback, response_ctx
        );
    }

    if (common_headers.message_type != EVENTSTREAM_APPLICATION_MESSAGE) {
        GGL_LOGE(
            "Unexpected message type %" PRId32 ".", common_headers.message_type
        );
        return GGL_ERR_FAILURE;
    }

    if (result_callback == NULL) {
        return GGL_ERR_OK;
    }

    GglArena alloc = ggl_arena_init(GGL_BUF(response_decode_mem));
    GglObject result = GGL_OBJ_NULL;

    GglError ret = ggl_json_decode_destructive(msg.payload, &alloc, &result);
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to decode IPC response payload.");
        return ret;
    }

    return result_callback(response_ctx, result);
}

typedef struct {
    pthread_mutex_t *mtx;
    pthread_cond_t *cond;
    bool ready;
    GglError ret;
    GgIpcResultCallback *result_callback;
    GgIpcErrorCallback *error_callback;
    void *response_ctx;
} CallCallbackCtx;

static GglError call_stream_handler(
    void *ctx, EventStreamCommonHeaders common_headers, EventStreamMessage msg
) {
    CallCallbackCtx *call_ctx = ctx;

    GGL_MTX_SCOPE_GUARD(call_ctx->mtx);

    call_ctx->ret = decode_response(
        common_headers,
        msg,
        call_ctx->result_callback,
        call_ctx->error_callback,
        call_ctx->response_ctx
    );

    call_ctx->ready = true;
    pthread_cond_signal(call_ctx->cond);

    return GGL_ERR_OK;
}

static void cleanup_pthread_cond(pthread_cond_t **cond) {
    pthread_cond_destroy(*cond);
}

GglError ggipc_call(
    GglBuffer operation,
    GglBuffer service_model_type,
    GglMap params,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_ctx
) {
    if (!connected()) {
        return GGL_ERR_NOCONN;
    }

    pthread_condattr_t notify_condattr;
    pthread_condattr_init(&notify_condattr);
    pthread_condattr_setclock(&notify_condattr, CLOCK_MONOTONIC);
    pthread_cond_t notify_cond;
    pthread_cond_init(&notify_cond, &notify_condattr);
    pthread_condattr_destroy(&notify_condattr);
    GGL_CLEANUP(cleanup_pthread_cond, &notify_cond);
    pthread_mutex_t notify_mtx = PTHREAD_MUTEX_INITIALIZER;

    CallCallbackCtx stream_handler_ctx = {
        .mtx = &notify_mtx,
        .cond = &notify_cond,
        .ready = false,
        .ret = GGL_ERR_TIMEOUT,
        .result_callback = result_callback,
        .error_callback = error_callback,
        .response_ctx = response_ctx,
    };

    int32_t stream_id;
    GglError ret = ggipc_set_stream_handler(
        &call_stream_handler, &stream_handler_ctx, &stream_id
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("GG-IPC subscribe call failed to get available stream id.");
        return ret;
    }

    EventStreamHeader headers[] = {
        { GGL_STR(":message-type"),
          { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_APPLICATION_MESSAGE } },
        { GGL_STR(":message-flags"), { EVENTSTREAM_INT32, .int32 = 0 } },
        { GGL_STR(":stream-id"), { EVENTSTREAM_INT32, .int32 = stream_id } },
        { GGL_STR("operation"), { EVENTSTREAM_STRING, .string = operation } },
        { GGL_STR("service-model-type"),
          { EVENTSTREAM_STRING, .string = service_model_type } },
    };
    size_t headers_len = sizeof(headers) / sizeof(headers[0]);

    GGL_MTX_SCOPE_GUARD(&call_state_mtx);

    ret = ggipc_send_message(
        conn_fd, headers, headers_len, &params, GGL_BUF(payload_array)
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to send message %d", ret);
        ggipc_set_stream_handler_at(stream_id, NULL, NULL);
        return ret;
    }

    struct timespec timeout;
    clock_gettime(CLOCK_MONOTONIC, &timeout);
    timeout.tv_sec += GGL_IPC_RESPONSE_TIMEOUT;

    {
        GGL_MTX_SCOPE_GUARD(&notify_mtx);

        while (!stream_handler_ctx.ready) {
            int cond_ret
                = pthread_cond_timedwait(&notify_cond, &notify_mtx, &timeout);
            if ((cond_ret != 0) && (cond_ret != EINTR)) {
                assert(cond_ret == ETIMEDOUT);
                GGL_LOGW("Timed out waiting for a response.");
                break;
            }
        }
    }

    // Even if we timed out, handler could still run. Clearing the stream
    // handler takes a mutex held when running handlers.

    ggipc_clear_stream_handler_if_eq(
        stream_id, &call_stream_handler, &stream_handler_ctx
    );

    // At this point we know handler has either run or will not run. Context
    // data must live to this point.

    return stream_handler_ctx.ret;
}

typedef struct {
    pthread_mutex_t *mtx;
    pthread_cond_t *cond;
    bool ready;
    GglError ret;
    GgIpcResultCallback *result_callback;
    GgIpcErrorCallback *error_callback;
    void *response_ctx;
    GgIpcSubscribeCallback *sub_callback;
    void *sub_callback_ctx;
} SubscribeCallbackCtx;

static GglError call_sub_callback(
    void *ctx, EventStreamCommonHeaders common_headers, EventStreamMessage msg
) {
    CallerSubHandler *sub_callback = ctx;
    if (common_headers.message_type != EVENTSTREAM_APPLICATION_MESSAGE) {
        GGL_LOGE(
            "Unexpected message type %" PRId32 " on stream %d.",
            common_headers.message_type,
            common_headers.stream_id
        );
        return GGL_ERR_FAILURE;
    }

    GglBuffer service_model_type = GGL_STR("");
    GglBuffer content_type = GGL_STR("");

    {
        EventStreamHeaderIter iter = msg.headers;
        EventStreamHeader header;

        while (eventstream_header_next(&iter, &header) == GGL_ERR_OK) {
            if (ggl_buffer_eq(header.name, GGL_STR("service-model-type"))) {
                if (header.value.type != EVENTSTREAM_STRING) {
                    GGL_LOGE("service-model-type header not string.");
                    return GGL_ERR_INVALID;
                }
                service_model_type = header.value.string;
            } else if (ggl_buffer_eq(header.name, GGL_STR(":content-type"))) {
                if (header.value.type != EVENTSTREAM_STRING) {
                    GGL_LOGE(":content-type header not string.");
                    return GGL_ERR_INVALID;
                }
                content_type = header.value.string;
            }
        }
    }

    if (!ggl_buffer_eq(content_type, GGL_STR("application/json"))) {
        GGL_LOGE(
            "Subscription response on stream %d does not declare a JSON "
            "payload.",
            common_headers.stream_id
        );
        return GGL_ERR_INVALID;
    }

    static uint8_t
        decode_mem[sizeof(GglObject) * GGL_IPC_PAYLOAD_MAX_SUBOBJECTS];
    GglArena decode_arena = ggl_arena_init(GGL_BUF(decode_mem));
    GglObject response;

    GglError ret
        = ggl_json_decode_destructive(msg.payload, &decode_arena, &response);
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to decode IPC response payload. Skipping.");
        return GGL_ERR_OK;
    }

    if (ggl_obj_type(response) != GGL_TYPE_MAP) {
        GGL_LOGE("IPC response payload JSON is not an object.");
        return GGL_ERR_OK;
    }

    return sub_callback->fn(
        sub_callback->ctx, service_model_type, ggl_obj_into_map(response)
    );
}

static GglError subscribe_stream_handler(
    void *ctx, EventStreamCommonHeaders common_headers, EventStreamMessage msg
) {
    SubscribeCallbackCtx *call_ctx = ctx;

    GGL_MTX_SCOPE_GUARD(call_ctx->mtx);

    call_ctx->ret = decode_response(
        common_headers,
        msg,
        call_ctx->result_callback,
        call_ctx->error_callback,
        call_ctx->response_ctx
    );

    if (call_ctx->ret != GGL_ERR_OK) {
        ggipc_set_stream_handler_at(common_headers.stream_id, NULL, NULL);
    } else {
        size_t index = 0;
        (void) ggipc_get_stream_index(common_headers.stream_id, &index);
        assert(index > 0);
        CallerSubHandler *caller_handler = &sub_callbacks[index - 1];
        caller_handler->fn = call_ctx->sub_callback;
        caller_handler->ctx = call_ctx->sub_callback_ctx;

        ggipc_set_stream_handler_at(
            common_headers.stream_id, &call_sub_callback, caller_handler
        );
    }

    call_ctx->ready = true;
    pthread_cond_signal(call_ctx->cond);

    return GGL_ERR_OK;
}

GglError ggipc_subscribe(
    GglBuffer operation,
    GglBuffer service_model_type,
    GglMap params,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_callback_ctx,
    GgIpcSubscribeCallback *sub_callback,
    void *sub_callback_ctx
) {
    if (!connected()) {
        return GGL_ERR_NOCONN;
    }

    pthread_condattr_t notify_condattr;
    pthread_condattr_init(&notify_condattr);
    pthread_condattr_setclock(&notify_condattr, CLOCK_MONOTONIC);
    pthread_cond_t notify_cond;
    pthread_cond_init(&notify_cond, &notify_condattr);
    pthread_condattr_destroy(&notify_condattr);
    GGL_CLEANUP(cleanup_pthread_cond, &notify_cond);
    pthread_mutex_t notify_mtx = PTHREAD_MUTEX_INITIALIZER;

    SubscribeCallbackCtx stream_resp_ctx = {
        .mtx = &notify_mtx,
        .cond = &notify_cond,
        .ready = false,
        .ret = GGL_ERR_TIMEOUT,
        .result_callback = result_callback,
        .error_callback = error_callback,
        .response_ctx = response_callback_ctx,
        .sub_callback = sub_callback,
        .sub_callback_ctx = sub_callback_ctx,
    };

    int32_t stream_id;
    GglError ret = ggipc_set_stream_handler(
        &subscribe_stream_handler, &stream_resp_ctx, &stream_id
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("GG-IPC subscribe call failed to get available stream id.");
        return ret;
    }

    EventStreamHeader headers[] = {
        { GGL_STR(":message-type"),
          { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_APPLICATION_MESSAGE } },
        { GGL_STR(":message-flags"), { EVENTSTREAM_INT32, .int32 = 0 } },
        { GGL_STR(":stream-id"), { EVENTSTREAM_INT32, .int32 = stream_id } },
        { GGL_STR("operation"), { EVENTSTREAM_STRING, .string = operation } },
        { GGL_STR("service-model-type"),
          { EVENTSTREAM_STRING, .string = service_model_type } },
    };
    size_t headers_len = sizeof(headers) / sizeof(headers[0]);

    GGL_MTX_SCOPE_GUARD(&call_state_mtx);

    ret = ggipc_send_message(
        conn_fd, headers, headers_len, &params, GGL_BUF(payload_array)
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to send message %d", ret);
        ggipc_set_stream_handler_at(stream_id, NULL, NULL);
        return ret;
    }

    struct timespec timeout;
    clock_gettime(CLOCK_MONOTONIC, &timeout);
    timeout.tv_sec += GGL_IPC_RESPONSE_TIMEOUT;

    {
        GGL_MTX_SCOPE_GUARD(&notify_mtx);

        while (!stream_resp_ctx.ready) {
            int cond_ret
                = pthread_cond_timedwait(&notify_cond, &notify_mtx, &timeout);
            if ((cond_ret != 0) && (cond_ret != EINTR)) {
                assert(cond_ret == ETIMEDOUT);
                GGL_LOGW("Timed out waiting for a response.");
                break;
            }
        }
    }

    // Even if we timed out, handler could still run. We only want to clear the
    // handler if the handler has not already changed it itself (either cleared
    // in case of error or else the handler for follow-up responses).

    ggipc_clear_stream_handler_if_eq(
        stream_id, &subscribe_stream_handler, &stream_resp_ctx
    );

    // At this point we know first handler has run or will not run. Stream may
    // be set to call the subscription response handler.

    return stream_resp_ctx.ret;
}
