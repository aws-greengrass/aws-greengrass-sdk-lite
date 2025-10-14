// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <errno.h>
#include <ggl/arena.h>
#include <ggl/attr.h>
#include <ggl/buffer.h>
#include <ggl/cleanup.h>
#include <ggl/error.h>
#include <ggl/eventstream/decode.h>
#include <ggl/eventstream/encode.h>
#include <ggl/eventstream/rpc.h>
#include <ggl/eventstream/types.h>
#include <ggl/file.h> // IWYU pragma: keep (TODO: remove after file.h refactor)
#include <ggl/flags.h>
#include <ggl/init.h>
#include <ggl/io.h>
#include <ggl/ipc/client.h>
#include <ggl/ipc/client_priv.h>
#include <ggl/ipc/client_raw.h>
#include <ggl/ipc/limits.h>
#include <ggl/json_decode.h>
#include <ggl/json_encode.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <ggl/socket.h>
#include <ggl/socket_epoll.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdnoreturn.h>

static atomic_int ipc_conn_fd = -1;

// Used while connecting or by receiving thread which are mutually exclusive.
static uint8_t ipc_recv_mem[GGL_IPC_MAX_MSG_LEN];
static uint8_t
    ipc_recv_decode_mem[sizeof(GglObject[GGL_MAX_OBJECT_SUBOBJECTS])];

static int epoll_fd = -1;
static pid_t recv_thread_id = -1;

typedef struct {
    void (*fn)(void);
    void *ctx;
} StreamHandler;

static_assert(
    GGL_IPC_MAX_STREAMS <= UINT16_MAX, "Max stream count must fit in 16 bits."
);

static int32_t stream_state_id[GGL_IPC_MAX_STREAMS] = { 0 };
static uint16_t stream_state_generation[GGL_IPC_MAX_STREAMS] = { 0 };
static StreamHandler stream_state_handler[GGL_IPC_MAX_STREAMS] = { 0 };

static pthread_mutex_t stream_state_mtx;

__attribute__((constructor)) static void init_stream_state_mtx(void) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&stream_state_mtx, &attr);
}

static GglError init_ipc_recv_thread(void);
ACCESS(none, 1)
noreturn static void *recv_thread(void *args);

__attribute__((constructor)) static void register_init_ipc_recv_thread(void) {
    static GglInitEntry entry = { .fn = &init_ipc_recv_thread };
    ggl_register_init_fn(&entry);
}

static GglError init_ipc_recv_thread(void) {
    GglError ret = ggl_socket_epoll_create(&epoll_fd);
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to create epoll for GG-IPC sockets.");
        return ret;
    }

    pthread_t recv_thread_handle;
    int sys_ret = pthread_create(&recv_thread_handle, NULL, &recv_thread, NULL);
    if (sys_ret != 0) {
        GGL_LOGE("Failed to create GG-IPC receive thread: %d.", sys_ret);
        return GGL_ERR_FATAL;
    }
    pthread_detach(recv_thread_handle);
    return GGL_ERR_OK;
}

// Requires holding stream_state_mtx
static GglError validate_handle(
    GgIpcSubscriptionHandle handle, uint16_t *index, const char *location
) {
    // Underflow ok; UINT16_MAX will fail bounds check
    uint16_t handle_index = (uint16_t) ((handle.val & UINT16_MAX) - 1U);
    uint16_t handle_generation = (uint16_t) (handle.val >> 16);

    if (handle_index >= GGL_IPC_MAX_STREAMS) {
        GGL_LOGE("Invalid handle %u in %s.", handle.val, location);
        return GGL_ERR_INVALID;
    }

    if (handle_generation != stream_state_generation[handle_index]) {
        GGL_LOGE(
            "Generation mismatch for handle %" PRIu32 " in %s.",
            handle.val,
            location
        );
        return GGL_ERR_NOENTRY;
    }

    *index = handle_index;
    return GGL_ERR_OK;
}

// Requires holding stream_state_mtx
static GgIpcSubscriptionHandle get_current_handle(uint16_t index) {
    assert(index < GGL_IPC_MAX_STREAMS);
    return (GgIpcSubscriptionHandle) {
        (uint32_t) stream_state_generation[index] << 16 | (index + 1U),
    };
}

// Requires holding stream_state_mtx
static bool get_stream_index_from_id(int32_t stream_id, uint16_t *index) {
    if (stream_id <= 0) {
        return false;
    }

    for (uint16_t i = 0; i < GGL_IPC_MAX_STREAMS; i++) {
        if (stream_state_id[i] == stream_id) {
            *index = i;
            return true;
        }
    }
    return false;
}

// Requires holding stream_state_mtx
static bool claim_stream_index(uint16_t *index) {
    for (uint16_t i = 0; i < GGL_IPC_MAX_STREAMS; i++) {
        if (stream_state_id[i] == 0) {
            stream_state_generation[i] += 1;
            stream_state_id[i] = -1;
            *index = i;
            return true;
        }
    }
    return false;
}

// Requires holding stream_state_mtx
static void set_stream_index(
    uint16_t index, int32_t stream_id, StreamHandler handler
) {
    stream_state_id[index] = stream_id;
    stream_state_handler[index] = handler;
}

// Requires holding stream_state_mtx
static void clear_stream_index(uint16_t index) {
    stream_state_generation[index] += 1;
    stream_state_id[index] = 0;
    stream_state_handler[index] = (StreamHandler) { 0 };
}

// After connected, requires holding stream_state_mtx
static GglError ipc_send_packet(
    int conn,
    const EventStreamHeader *headers,
    size_t headers_len,
    GglReader payload
) {
    static uint8_t ipc_send_mem[GGL_IPC_MAX_MSG_LEN];
    GglBuffer es_packet = GGL_BUF(ipc_send_mem);

    GglError ret
        = eventstream_encode(&es_packet, headers, headers_len, payload);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    return ggl_socket_write(conn, es_packet);
}

static bool connected(void) {
    return ipc_conn_fd >= 0;
}

static GglError register_ipc_socket(int conn) {
    assert(epoll_fd >= 0);
    return ggl_socket_epoll_add(epoll_fd, conn, (uint64_t) conn);
}

__attribute__((weak)) GglError
ggipc_connect_extra_header_handler(EventStreamHeaderIter headers) {
    (void) headers;
    return GGL_ERR_OK;
}

GglError ggipc_connect_with_payload(GglBuffer socket_path, GglObject payload) {
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

    ret = ipc_send_packet(
        conn, headers, headers_len, ggl_json_reader(&payload)
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to send GG-IPC connect packet on fd %d.", conn);
        return ret;
    }

    EventStreamMessage msg = { 0 };
    ret = eventsteam_get_packet(
        ggl_socket_reader(&conn), &msg, GGL_BUF(ipc_recv_mem)
    );
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

    ret = ggipc_connect_extra_header_handler(msg.headers);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    ret = register_ipc_socket(conn);
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to register GG-IPC fd %d for receiving.", conn);
        return ret;
    }

    conn_cleanup = -1;
    ipc_conn_fd = conn;

    return GGL_ERR_OK;
}

GglError ggipc_connect_with_token(GglBuffer socket_path, GglBuffer auth_token) {
    return ggipc_connect_with_payload(
        socket_path,
        ggl_obj_map(
            GGL_MAP(ggl_kv(GGL_STR("authToken"), ggl_obj_buf(auth_token)))
        )
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

static void cleanup_pthread_cond(pthread_cond_t **cond) {
    pthread_cond_destroy(*cond);
}

// Must hold stream_state_mtx
static GglError handle_application_error(
    GglBuffer payload, GgIpcErrorCallback *error_callback, void *response_ctx
) {
    if (error_callback == NULL) {
        return GGL_ERR_REMOTE;
    }

    GglArena error_alloc = ggl_arena_init(GGL_BUF(ipc_recv_decode_mem));

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

static GglError response_handler_inner(
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
            "Unexpected message type %" PRId32 " on stream %" PRId32 ".",
            common_headers.message_type,
            common_headers.stream_id
        );
        return GGL_ERR_FAILURE;
    }

    if (result_callback == NULL) {
        return GGL_ERR_OK;
    }

    GglArena alloc = ggl_arena_init(GGL_BUF(ipc_recv_decode_mem));
    GglObject result = GGL_OBJ_NULL;

    GglError ret = ggl_json_decode_destructive(msg.payload, &alloc, &result);
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to decode IPC response payload.");
        return ret;
    }

    if (ggl_obj_type(result) != GGL_TYPE_MAP) {
        GGL_LOGE("IPC response payload is not a JSON object.");
        return GGL_ERR_FAILURE;
    }

    return result_callback(response_ctx, ggl_obj_into_map(result));
}

typedef struct {
    pthread_cond_t *cond;
    bool ready;
    GglError ret;
    GgIpcResultCallback *result_callback;
    GgIpcErrorCallback *error_callback;
    void *response_ctx;
    GgIpcSubscribeCallback *sub_callback;
    void *sub_callback_ctx;
} ResponseHandlerCtx;

// Must hold stream_state_mtx
static void response_handler(
    uint16_t index,
    void *ctx,
    EventStreamCommonHeaders common_headers,
    EventStreamMessage msg
) {
    ResponseHandlerCtx *call_ctx = ctx;

    call_ctx->ret = response_handler_inner(
        common_headers,
        msg,
        call_ctx->result_callback,
        call_ctx->error_callback,
        call_ctx->response_ctx
    );

    if ((call_ctx->sub_callback == NULL) || (call_ctx->ret != GGL_ERR_OK)) {
        clear_stream_index(index);
    } else {
        if ((common_headers.message_flags & EVENTSTREAM_TERMINATE_STREAM)
            != 0) {
            GGL_LOGE(
                "Terminate stream received on stream_id %" PRIi32
                " for initial subscription response.",
                common_headers.stream_id
            );
            clear_stream_index(index);
            call_ctx->ret = GGL_ERR_FAILURE;
        } else {
            set_stream_index(
                index,
                common_headers.stream_id,
                (StreamHandler) {
                    .fn = (void (*)(void)) call_ctx->sub_callback,
                    .ctx = call_ctx->sub_callback_ctx,
                }
            );
        }
    }

    call_ctx->ready = true;
    pthread_cond_signal(call_ctx->cond);
}

GglError ggipc_call(
    GglBuffer operation,
    GglBuffer service_model_type,
    GglMap params,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_ctx
) {
    return ggipc_subscribe(
        operation,
        service_model_type,
        params,
        result_callback,
        error_callback,
        response_ctx,
        NULL,
        NULL,
        NULL
    );
}

GglError ggipc_subscribe(
    GglBuffer operation,
    GglBuffer service_model_type,
    GglMap params,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_ctx,
    GgIpcSubscribeCallback *sub_callback,
    void *sub_callback_ctx,
    GgIpcSubscriptionHandle *sub_handle
) {
    if (!connected()) {
        return GGL_ERR_NOCONN;
    }

    if (recv_thread_id == gettid()) {
        GGL_LOGE(
            "GG IPC calls may not be made from within subscription callbacks."
        );
        return GGL_ERR_INVALID;
    }

    pthread_condattr_t notify_condattr;
    pthread_condattr_init(&notify_condattr);
    pthread_condattr_setclock(&notify_condattr, CLOCK_MONOTONIC);
    pthread_cond_t notify_cond;
    pthread_cond_init(&notify_cond, &notify_condattr);
    pthread_condattr_destroy(&notify_condattr);
    GGL_CLEANUP(cleanup_pthread_cond, &notify_cond);

    ResponseHandlerCtx response_handler_ctx = {
        .cond = &notify_cond,
        .ready = false,
        .ret = GGL_ERR_TIMEOUT,
        .result_callback = result_callback,
        .error_callback = error_callback,
        .response_ctx = response_ctx,
        .sub_callback = sub_callback,
        .sub_callback_ctx = sub_callback_ctx,
    };

    uint16_t stream_index;
    int32_t stream_id = -1;

    GGL_MTX_SCOPE_GUARD(&stream_state_mtx);

    bool index_available = claim_stream_index(&stream_index);
    if (!index_available) {
        GGL_LOGE("GG-IPC request failed to get available stream slot.");
        return GGL_ERR_NOMEM;
    }

    static int32_t next_stream_id = 1;

    stream_id = next_stream_id++;

    set_stream_index(
        stream_index,
        stream_id,
        (StreamHandler) {
            .fn = (void (*)(void)) response_handler,
            .ctx = &response_handler_ctx,
        }
    );

    if (sub_handle != NULL) {
        *sub_handle = get_current_handle(stream_index);
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

    GglObject params_obj = ggl_obj_map(params);
    GglError ret = ipc_send_packet(
        ipc_conn_fd, headers, headers_len, ggl_json_reader(&params_obj)
    );

    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to send EventStream packet.");
        clear_stream_index(stream_index);
        return ret;
    }

    struct timespec timeout;
    clock_gettime(CLOCK_MONOTONIC, &timeout);
    timeout.tv_sec += GGL_IPC_RESPONSE_TIMEOUT;

    while (!response_handler_ctx.ready) {
        int cond_ret
            = pthread_cond_timedwait(&notify_cond, &stream_state_mtx, &timeout);
        if ((cond_ret != 0) && (cond_ret != EINTR)) {
            assert(cond_ret == ETIMEDOUT);
            GGL_LOGW("Timed out waiting for a response.");
            clear_stream_index(stream_index);
            return GGL_ERR_TIMEOUT;
        }
    }

    return response_handler_ctx.ret;
}

// Must hold stream_state_mtx
static GglError call_sub_callback(
    GgIpcSubscriptionHandle handle,
    GgIpcSubscribeCallback *sub_callback,
    void *sub_callback_ctx,
    EventStreamCommonHeaders common_headers,
    EventStreamMessage msg
) {
    if (common_headers.message_type != EVENTSTREAM_APPLICATION_MESSAGE) {
        GGL_LOGE(
            "Unexpected message type %" PRId32 " on stream %" PRId32 ".",
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
            "Subscription response on stream %" PRId32
            " does not declare a JSON payload.",
            common_headers.stream_id
        );
        return GGL_ERR_INVALID;
    }

    GglArena arena = ggl_arena_init(GGL_BUF(ipc_recv_decode_mem));
    GglObject response;

    GglError ret = ggl_json_decode_destructive(msg.payload, &arena, &response);
    if (ret == GGL_ERR_NOMEM) {
        GGL_LOGE(
            "IPC response payload too large on stream %" PRId32 ". Skipping.",
            common_headers.stream_id
        );
        return GGL_ERR_OK;
    }
    if (ret != GGL_ERR_OK) {
        GGL_LOGE(
            "Failed to decode IPC response payload on stream %" PRId32 ".",
            common_headers.stream_id
        );
        return ret;
    }

    if (ggl_obj_type(response) != GGL_TYPE_MAP) {
        GGL_LOGE("IPC response payload JSON is not an object.");
        return GGL_ERR_INVALID;
    }

    return sub_callback(
        sub_callback_ctx, handle, service_model_type, ggl_obj_into_map(response)
    );
}

static GglError dispatch_incoming_packet(int conn) {
    EventStreamMessage msg;
    GglError ret = eventsteam_get_packet(
        ggl_socket_reader(&conn), &msg, GGL_BUF(ipc_recv_mem)
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed to read eventstream packet.");
        return ret;
    }

    EventStreamCommonHeaders common_headers;
    ret = eventstream_get_common_headers(&msg, &common_headers);
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Eventstream packet missing required headers.");
        return ret;
    }

    int32_t stream_id = common_headers.stream_id;

    if (stream_id < 0) {
        GGL_LOGE("Eventstream packet has negative stream id.");
        return GGL_ERR_FAILURE;
    }

    GGL_MTX_SCOPE_GUARD(&stream_state_mtx);

    uint16_t index;
    bool found = get_stream_index_from_id(stream_id, &index);

    if (!found) {
        GGL_LOGE(
            "Unhandled eventstream packet with stream id %" PRId32 " dropped.",
            stream_id
        );
        return GGL_ERR_OK;
    }

    assert(stream_state_handler[index].fn != NULL);

    if (stream_state_handler[index].fn == (void (*)(void)) response_handler) {
        // Must hold stream_state_mtx through handler call.
        response_handler(
            index, stream_state_handler[index].ctx, common_headers, msg
        );
        return GGL_ERR_OK;
    }

    GglError sub_ret = call_sub_callback(
        get_current_handle(index),
        (GgIpcSubscribeCallback *) stream_state_handler[index].fn,
        stream_state_handler[index].ctx,
        common_headers,
        msg
    );
    if ((sub_ret != GGL_ERR_OK)
        || ((common_headers.message_flags & EVENTSTREAM_TERMINATE_STREAM)
            != 0)) {
        GGL_LOGD("Closing stream %" PRIi32 " for %d", stream_id, conn);
        clear_stream_index(index);
    }

    return GGL_ERR_OK;
}

ACCESS(none, 1)
static GglError data_ready_callback(void *ctx, uint64_t data) {
    (void) ctx;
    (void) data;

    GglError ret = dispatch_incoming_packet(ipc_conn_fd);

    if (ret != GGL_ERR_OK) {
        GGL_LOGE(
            "Error receiving from GG-IPC connection on fd %d. Closing "
            "connection.",
            ipc_conn_fd
        );
        (void) ggl_close(ipc_conn_fd);
    }

    return ret;
}

noreturn static void *recv_thread(void *args) {
    (void) args;

    GGL_LOGI("Starting GG-IPC receive thread.");

    recv_thread_id = gettid();

    (void) ggl_socket_epoll_run(epoll_fd, &data_ready_callback, NULL);

    GGL_LOGE("GG-IPC receive thread failed. Exiting.");
    _Exit(1);
}

void ggipc_close_subscription(GgIpcSubscriptionHandle handle) {
    GGL_MTX_SCOPE_GUARD(&stream_state_mtx);

    uint16_t index;
    GglError ret = validate_handle(handle, &index, __func__);
    if (ret != GGL_ERR_OK) {
        return;
    }

    clear_stream_index(index);
}
