// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <errno.h>
#include <gg/arena.h>
#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/cleanup.h>
#include <gg/error.h>
#include <gg/eventstream/decode.h>
#include <gg/eventstream/encode.h>
#include <gg/eventstream/rpc.h>
#include <gg/eventstream/types.h>
#include <gg/file.h> // IWYU pragma: keep (TODO: remove after file.h refactor)
#include <gg/flags.h>
#include <gg/init.h>
#include <gg/io.h>
#include <gg/ipc/client.h>
#include <gg/ipc/client_priv.h>
#include <gg/ipc/client_raw.h>
#include <gg/ipc/limits.h>
#include <gg/json_decode.h>
#include <gg/json_encode.h>
#include <gg/log.h>
#include <gg/map.h>
#include <gg/object.h>
#include <gg/socket.h>
#include <gg/socket_epoll.h>
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
static uint8_t ipc_recv_mem[GG_IPC_MAX_MSG_LEN];
static uint8_t ipc_recv_decode_mem[sizeof(GgObject[GG_MAX_OBJECT_SUBOBJECTS])];

static int epoll_fd = -1;
static pid_t recv_thread_id = -1;

typedef struct {
    GgIpcSubscribeCallback *fn;
    void *ctx;
    void *aux_ctx;
} StreamHandler;

static_assert(
    GG_IPC_MAX_STREAMS <= UINT16_MAX, "Max stream count must fit in 16 bits."
);

static int32_t stream_state_id[GG_IPC_MAX_STREAMS] = { 0 };
static uint16_t stream_state_generation[GG_IPC_MAX_STREAMS] = { 0 };
static StreamHandler stream_state_handler[GG_IPC_MAX_STREAMS] = { 0 };

static pthread_mutex_t stream_state_mtx;

__attribute__((constructor)) static void init_stream_state_mtx(void) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&stream_state_mtx, &attr);
}

static GgError init_ipc_recv_thread(void);
ACCESS(none, 1)
noreturn static void *recv_thread(void *args);

__attribute__((constructor)) static void register_init_ipc_recv_thread(void) {
    static GgInitEntry entry = { .fn = &init_ipc_recv_thread };
    gg_register_init_fn(&entry);
}

static GgError init_ipc_recv_thread(void) {
    GgError ret = gg_socket_epoll_create(&epoll_fd);
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to create epoll for GG-IPC sockets.");
        return ret;
    }

    pthread_t recv_thread_handle;
    int sys_ret = pthread_create(&recv_thread_handle, NULL, &recv_thread, NULL);
    if (sys_ret != 0) {
        GG_LOGE("Failed to create GG-IPC receive thread: %d.", sys_ret);
        return GG_ERR_FATAL;
    }
    pthread_detach(recv_thread_handle);
    return GG_ERR_OK;
}

// Requires holding stream_state_mtx
static GgError validate_handle(
    GgIpcSubscriptionHandle handle, uint16_t *index, const char *location
) {
    // Underflow ok; UINT16_MAX will fail bounds check
    uint16_t handle_index = (uint16_t) ((handle.val & UINT16_MAX) - 1U);
    uint16_t handle_generation = (uint16_t) (handle.val >> 16);

    if (handle_index >= GG_IPC_MAX_STREAMS) {
        GG_LOGE("Invalid handle %u in %s.", handle.val, location);
        return GG_ERR_INVALID;
    }

    if (handle_generation != stream_state_generation[handle_index]) {
        GG_LOGE(
            "Generation mismatch for handle %" PRIu32 " in %s.",
            handle.val,
            location
        );
        return GG_ERR_NOENTRY;
    }

    *index = handle_index;
    return GG_ERR_OK;
}

// Requires holding stream_state_mtx
static GgIpcSubscriptionHandle get_current_handle(uint16_t index) {
    assert(index < GG_IPC_MAX_STREAMS);
    return (GgIpcSubscriptionHandle) {
        (uint32_t) stream_state_generation[index] << 16 | (index + 1U),
    };
}

// Requires holding stream_state_mtx
static bool get_stream_index_from_id(int32_t stream_id, uint16_t *index) {
    if (stream_id <= 0) {
        return false;
    }

    for (uint16_t i = 0; i < GG_IPC_MAX_STREAMS; i++) {
        if (stream_state_id[i] == stream_id) {
            *index = i;
            return true;
        }
    }
    return false;
}

// Requires holding stream_state_mtx
static bool claim_stream_index(uint16_t *index) {
    for (uint16_t i = 0; i < GG_IPC_MAX_STREAMS; i++) {
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
static GgError ipc_send_packet(
    int conn,
    const EventStreamHeader *headers,
    size_t headers_len,
    GgReader payload
) {
    static uint8_t ipc_send_mem[GG_IPC_MAX_MSG_LEN];
    GgBuffer es_packet = GG_BUF(ipc_send_mem);

    GgError ret = eventstream_encode(&es_packet, headers, headers_len, payload);
    if (ret != GG_ERR_OK) {
        return ret;
    }

    return gg_socket_write(conn, es_packet);
}

static bool connected(void) {
    return ipc_conn_fd >= 0;
}

static GgError register_ipc_socket(int conn) {
    assert(epoll_fd >= 0);
    return gg_socket_epoll_add(epoll_fd, conn, (uint64_t) conn);
}

__attribute__((weak)) GgError
ggipc_connect_extra_header_handler(EventStreamHeaderIter headers) {
    (void) headers;
    return GG_ERR_OK;
}

GgError ggipc_connect_with_payload(GgBuffer socket_path, GgObject payload) {
    assert(!connected());

    int conn = -1;
    GgError ret = gg_connect(socket_path, &conn);
    if (ret != GG_ERR_OK) {
        GG_LOGE(
            "Failed to connect to GG-IPC socket at %.*s.",
            (int) socket_path.len,
            socket_path.data
        );
        return ret;
    }
    GG_CLEANUP_ID(conn_cleanup, cleanup_close, conn);

    GG_LOGI(
        "Connected to GG-IPC socket at %.*s on fd %d",
        (int) socket_path.len,
        socket_path.data,
        conn
    );

    EventStreamHeader headers[] = {
        { GG_STR(":message-type"),
          { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_CONNECT } },
        { GG_STR(":message-flags"), { EVENTSTREAM_INT32, .int32 = 0 } },
        { GG_STR(":stream-id"), { EVENTSTREAM_INT32, .int32 = 0 } },
        { GG_STR(":version"),
          { EVENTSTREAM_STRING, .string = GG_STR("0.1.0") } },
    };
    size_t headers_len = sizeof(headers) / sizeof(headers[0]);

    ret = ipc_send_packet(conn, headers, headers_len, gg_json_reader(&payload));
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to send GG-IPC connect packet on fd %d.", conn);
        return ret;
    }

    EventStreamMessage msg = { 0 };
    ret = eventsteam_get_packet(
        gg_socket_reader(&conn), &msg, GG_BUF(ipc_recv_mem)
    );
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to receive GG-IPC connect ack on fd %d.", conn);
        return ret;
    }

    EventStreamCommonHeaders common_headers;
    ret = eventstream_get_common_headers(&msg, &common_headers);
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to parse response headers on GG-IPC fd %d.", conn);
        return ret;
    }

    if (common_headers.message_type != EVENTSTREAM_CONNECT_ACK) {
        GG_LOGE("GG-IPC fd %d connection response not an ack.", conn);
        return GG_ERR_FAILURE;
    }

    if ((common_headers.message_flags & EVENTSTREAM_CONNECTION_ACCEPTED) == 0) {
        GG_LOGE(
            "GG-IPC fd %d connection response missing accepted flag.", conn
        );
        return GG_ERR_FAILURE;
    }

    if (msg.payload.len != 0) {
        GG_LOGW(
            "GG-IPC fd %d eventstream connection ack has unexpected payload.",
            conn
        );
    }

    ret = ggipc_connect_extra_header_handler(msg.headers);
    if (ret != GG_ERR_OK) {
        return ret;
    }

    ret = register_ipc_socket(conn);
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to register GG-IPC fd %d for receiving.", conn);
        return ret;
    }

    conn_cleanup = -1;
    ipc_conn_fd = conn;

    return GG_ERR_OK;
}

GgError ggipc_connect_with_token(GgBuffer socket_path, GgBuffer auth_token) {
    return ggipc_connect_with_payload(
        socket_path,
        gg_obj_map(GG_MAP(gg_kv(GG_STR("authToken"), gg_obj_buf(auth_token))))
    );
}

GgError ggipc_connect(void) {
    // Unsafe, but function is documented as such
    // NOLINTBEGIN(concurrency-mt-unsafe)
    char *svcuid = getenv("SVCUID");
    if (svcuid == NULL) {
        return GG_ERR_CONFIG;
    }
    char *socket_path
        = getenv("AWS_GG_NUCLEUS_DOMAIN_SOCKET_FILEPATH_FOR_COMPONENT");
    if (socket_path == NULL) {
        return GG_ERR_CONFIG;
    }

    return ggipc_connect_with_token(
        gg_buffer_from_null_term(socket_path), gg_buffer_from_null_term(svcuid)
    );
    // NOLINTEND(concurrency-mt-unsafe)
}

static void cleanup_pthread_cond(pthread_cond_t **cond) {
    pthread_cond_destroy(*cond);
}

// Must hold stream_state_mtx
static GgError handle_application_error(
    GgBuffer payload, GgIpcErrorCallback *error_callback, void *response_ctx
) {
    if (error_callback == NULL) {
        return GG_ERR_REMOTE;
    }

    GgArena error_alloc = gg_arena_init(GG_BUF(ipc_recv_decode_mem));

    GgObject err_result;
    GgError ret
        = gg_json_decode_destructive(payload, &error_alloc, &err_result);
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to decode IPC error payload.");
        return ret;
    }
    if (gg_obj_type(err_result) != GG_TYPE_MAP) {
        GG_LOGE("Failed to decode IPC error payload.");
        return GG_ERR_PARSE;
    }

    GgObject *error_code_obj;
    GgObject *message_obj;

    ret = gg_map_validate(
        gg_obj_into_map(err_result),
        GG_MAP_SCHEMA(
            { GG_STR("_errorCode"), GG_REQUIRED, GG_TYPE_BUF, &error_code_obj },
            { GG_STR("_message"), GG_OPTIONAL, GG_TYPE_BUF, &message_obj },
        )
    );
    if (ret != GG_ERR_OK) {
        GG_LOGE("Error response does not match known schema.");
        return ret;
    }
    GgBuffer error_code = gg_obj_into_buf(*error_code_obj);

    GgBuffer message = GG_STR("");
    if (message_obj != NULL) {
        message = gg_obj_into_buf(*message_obj);
    }

    ret = error_callback(response_ctx, error_code, message);
    if (ret != GG_ERR_OK) {
        return ret;
    }

    return GG_ERR_REMOTE;
}

static GgError response_handler_inner(
    EventStreamCommonHeaders common_headers,
    EventStreamMessage msg,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_ctx
) {
    if (common_headers.message_type == EVENTSTREAM_APPLICATION_ERROR) {
        GG_LOGD(
            "Received an IPC error on stream %" PRId32 ".",
            common_headers.stream_id
        );

        return handle_application_error(
            msg.payload, error_callback, response_ctx
        );
    }

    if (common_headers.message_type != EVENTSTREAM_APPLICATION_MESSAGE) {
        GG_LOGE(
            "Unexpected message type %" PRId32 " on stream %" PRId32 ".",
            common_headers.message_type,
            common_headers.stream_id
        );
        return GG_ERR_FAILURE;
    }

    if (result_callback == NULL) {
        return GG_ERR_OK;
    }

    GgArena alloc = gg_arena_init(GG_BUF(ipc_recv_decode_mem));
    GgObject result = GG_OBJ_NULL;

    GgError ret = gg_json_decode_destructive(msg.payload, &alloc, &result);
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to decode IPC response payload.");
        return ret;
    }

    if (gg_obj_type(result) != GG_TYPE_MAP) {
        GG_LOGE("IPC response payload is not a JSON object.");
        return GG_ERR_FAILURE;
    }

    return result_callback(response_ctx, gg_obj_into_map(result));
}

typedef struct {
    pthread_cond_t *cond;
    bool ready;
    GgError ret;
    GgIpcResultCallback *result_callback;
    GgIpcErrorCallback *error_callback;
    void *response_ctx;
    GgIpcSubscribeCallback *sub_callback;
    void *sub_callback_ctx;
    void *sub_callback_aux_ctx;
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

    if ((call_ctx->sub_callback == NULL) || (call_ctx->ret != GG_ERR_OK)) {
        clear_stream_index(index);
    } else {
        if ((common_headers.message_flags & EVENTSTREAM_TERMINATE_STREAM)
            != 0) {
            GG_LOGE(
                "Terminate stream received on stream_id %" PRIi32
                " for initial subscription response.",
                common_headers.stream_id
            );
            clear_stream_index(index);
            call_ctx->ret = GG_ERR_FAILURE;
        } else {
            set_stream_index(
                index,
                common_headers.stream_id,
                (StreamHandler) {
                    .fn = call_ctx->sub_callback,
                    .ctx = call_ctx->sub_callback_ctx,
                    .aux_ctx = call_ctx->sub_callback_aux_ctx,
                }
            );
        }
    }

    call_ctx->ready = true;
    pthread_cond_signal(call_ctx->cond);
}

GgError ggipc_call(
    GgBuffer operation,
    GgBuffer service_model_type,
    GgMap params,
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
        NULL,
        NULL
    );
}

GgError ggipc_subscribe(
    GgBuffer operation,
    GgBuffer service_model_type,
    GgMap params,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_ctx,
    GgIpcSubscribeCallback *sub_callback,
    void *sub_callback_ctx,
    void *sub_callback_aux_ctx,
    GgIpcSubscriptionHandle *sub_handle
) {
    if (!connected()) {
        return GG_ERR_NOCONN;
    }

    if (recv_thread_id == gettid()) {
        GG_LOGE(
            "GG IPC calls may not be made from within subscription callbacks."
        );
        return GG_ERR_INVALID;
    }

    pthread_condattr_t notify_condattr;
    pthread_condattr_init(&notify_condattr);
    pthread_condattr_setclock(&notify_condattr, CLOCK_MONOTONIC);
    pthread_cond_t notify_cond;
    pthread_cond_init(&notify_cond, &notify_condattr);
    pthread_condattr_destroy(&notify_condattr);
    GG_CLEANUP(cleanup_pthread_cond, &notify_cond);

    ResponseHandlerCtx response_handler_ctx = {
        .cond = &notify_cond,
        .ready = false,
        .ret = GG_ERR_TIMEOUT,
        .result_callback = result_callback,
        .error_callback = error_callback,
        .response_ctx = response_ctx,
        .sub_callback = sub_callback,
        .sub_callback_ctx = sub_callback_ctx,
        .sub_callback_aux_ctx = sub_callback_aux_ctx,
    };

    uint16_t stream_index;
    int32_t stream_id = -1;

    GG_MTX_SCOPE_GUARD(&stream_state_mtx);

    bool index_available = claim_stream_index(&stream_index);
    if (!index_available) {
        GG_LOGE("GG-IPC request failed to get available stream slot.");
        return GG_ERR_NOMEM;
    }

    static int32_t next_stream_id = 1;

    stream_id = next_stream_id++;

    set_stream_index(
        stream_index,
        stream_id,
        (StreamHandler) {
            .fn = NULL,
            .ctx = &response_handler_ctx,
        }
    );

    if (sub_handle != NULL) {
        *sub_handle = get_current_handle(stream_index);
    }

    EventStreamHeader headers[] = {
        { GG_STR(":message-type"),
          { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_APPLICATION_MESSAGE } },
        { GG_STR(":message-flags"), { EVENTSTREAM_INT32, .int32 = 0 } },
        { GG_STR(":stream-id"), { EVENTSTREAM_INT32, .int32 = stream_id } },
        { GG_STR("operation"), { EVENTSTREAM_STRING, .string = operation } },
        { GG_STR("service-model-type"),
          { EVENTSTREAM_STRING, .string = service_model_type } },
    };
    size_t headers_len = sizeof(headers) / sizeof(headers[0]);

    GgObject params_obj = gg_obj_map(params);
    GgError ret = ipc_send_packet(
        ipc_conn_fd, headers, headers_len, gg_json_reader(&params_obj)
    );

    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to send EventStream packet.");
        clear_stream_index(stream_index);
        return ret;
    }

    struct timespec timeout;
    clock_gettime(CLOCK_MONOTONIC, &timeout);
    timeout.tv_sec += GG_IPC_RESPONSE_TIMEOUT;

    while (!response_handler_ctx.ready) {
        int cond_ret
            = pthread_cond_timedwait(&notify_cond, &stream_state_mtx, &timeout);
        if ((cond_ret != 0) && (cond_ret != EINTR)) {
            assert(cond_ret == ETIMEDOUT);
            GG_LOGW("Timed out waiting for a response.");
            clear_stream_index(stream_index);
            return GG_ERR_TIMEOUT;
        }
    }

    return response_handler_ctx.ret;
}

// Must hold stream_state_mtx
static GgError call_sub_callback(
    GgIpcSubscriptionHandle handle,
    GgIpcSubscribeCallback *sub_callback,
    void *sub_callback_ctx,
    void *sub_callback_aux_ctx,
    EventStreamCommonHeaders common_headers,
    EventStreamMessage msg
) {
    if (common_headers.message_type != EVENTSTREAM_APPLICATION_MESSAGE) {
        GG_LOGE(
            "Unexpected message type %" PRId32 " on stream %" PRId32 ".",
            common_headers.message_type,
            common_headers.stream_id
        );
        return GG_ERR_FAILURE;
    }

    GgBuffer service_model_type = GG_STR("");
    GgBuffer content_type = GG_STR("");

    {
        EventStreamHeaderIter iter = msg.headers;
        EventStreamHeader header;

        while (eventstream_header_next(&iter, &header) == GG_ERR_OK) {
            if (gg_buffer_eq(header.name, GG_STR("service-model-type"))) {
                if (header.value.type != EVENTSTREAM_STRING) {
                    GG_LOGE("service-model-type header not string.");
                    return GG_ERR_INVALID;
                }
                service_model_type = header.value.string;
            } else if (gg_buffer_eq(header.name, GG_STR(":content-type"))) {
                if (header.value.type != EVENTSTREAM_STRING) {
                    GG_LOGE(":content-type header not string.");
                    return GG_ERR_INVALID;
                }
                content_type = header.value.string;
            }
        }
    }

    if (!gg_buffer_eq(content_type, GG_STR("application/json"))) {
        GG_LOGE(
            "Subscription response on stream %" PRId32
            " does not declare a JSON payload.",
            common_headers.stream_id
        );
        return GG_ERR_INVALID;
    }

    GgArena arena = gg_arena_init(GG_BUF(ipc_recv_decode_mem));
    GgObject response;

    GgError ret = gg_json_decode_destructive(msg.payload, &arena, &response);
    if (ret == GG_ERR_NOMEM) {
        GG_LOGE(
            "IPC response payload too large on stream %" PRId32 ". Skipping.",
            common_headers.stream_id
        );
        return GG_ERR_OK;
    }
    if (ret != GG_ERR_OK) {
        GG_LOGE(
            "Failed to decode IPC response payload on stream %" PRId32 ".",
            common_headers.stream_id
        );
        return ret;
    }

    if (gg_obj_type(response) != GG_TYPE_MAP) {
        GG_LOGE("IPC response payload JSON is not an object.");
        return GG_ERR_INVALID;
    }

    return sub_callback(
        sub_callback_ctx,
        sub_callback_aux_ctx,
        handle,
        service_model_type,
        gg_obj_into_map(response)
    );
}

static GgError dispatch_incoming_packet(int conn) {
    EventStreamMessage msg;
    GgError ret = eventsteam_get_packet(
        gg_socket_reader(&conn), &msg, GG_BUF(ipc_recv_mem)
    );
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to read eventstream packet.");
        return ret;
    }

    EventStreamCommonHeaders common_headers;
    ret = eventstream_get_common_headers(&msg, &common_headers);
    if (ret != GG_ERR_OK) {
        GG_LOGE("Eventstream packet missing required headers.");
        return ret;
    }

    int32_t stream_id = common_headers.stream_id;

    if (stream_id < 0) {
        GG_LOGE("Eventstream packet has negative stream id.");
        return GG_ERR_FAILURE;
    }

    GG_MTX_SCOPE_GUARD(&stream_state_mtx);

    uint16_t index;
    bool found = get_stream_index_from_id(stream_id, &index);

    if (!found) {
        GG_LOGE(
            "Unhandled eventstream packet with stream id %" PRId32 " dropped.",
            stream_id
        );
        return GG_ERR_OK;
    }

    if (stream_state_handler[index].fn == NULL) {
        // Must hold stream_state_mtx through handler call.
        response_handler(
            index, stream_state_handler[index].ctx, common_headers, msg
        );
        return GG_ERR_OK;
    }

    GgError sub_ret = call_sub_callback(
        get_current_handle(index),
        stream_state_handler[index].fn,
        stream_state_handler[index].ctx,
        stream_state_handler[index].aux_ctx,
        common_headers,
        msg
    );
    if ((sub_ret != GG_ERR_OK)
        || ((common_headers.message_flags & EVENTSTREAM_TERMINATE_STREAM)
            != 0)) {
        GG_LOGD("Closing stream %" PRIi32 " for %d", stream_id, conn);
        clear_stream_index(index);
    }

    return GG_ERR_OK;
}

ACCESS(none, 1)
static GgError data_ready_callback(void *ctx, uint64_t data) {
    (void) ctx;
    (void) data;

    GgError ret = dispatch_incoming_packet(ipc_conn_fd);

    if (ret != GG_ERR_OK) {
        GG_LOGE(
            "Error receiving from GG-IPC connection on fd %d. Closing connection.",
            ipc_conn_fd
        );
        (void) gg_close(ipc_conn_fd);
    }

    return ret;
}

noreturn static void *recv_thread(void *args) {
    (void) args;

    GG_LOGI("Starting GG-IPC receive thread.");

    recv_thread_id = gettid();

    (void) gg_socket_epoll_run(epoll_fd, &data_ready_callback, NULL);

    GG_LOGE("GG-IPC receive thread failed. Exiting.");
    _Exit(1);
}

void ggipc_close_subscription(GgIpcSubscriptionHandle handle) {
    GG_MTX_SCOPE_GUARD(&stream_state_mtx);

    uint16_t index;
    GgError ret = validate_handle(handle, &index, __func__);
    if (ret != GG_ERR_OK) {
        return;
    }

    int32_t stream_id = stream_state_id[index];
    EventStreamHeader headers[] = {
        { GG_STR(":message-type"),
          { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_APPLICATION_MESSAGE } },
        { GG_STR(":message-flags"),
          { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_TERMINATE_STREAM } },
        { GG_STR(":stream-id"), { EVENTSTREAM_INT32, .int32 = stream_id } },
    };
    size_t headers_len = sizeof(headers) / sizeof(headers[0]);

    GG_LOGD(
        "Sending subscription termination for stream id %" PRIi32 ".", stream_id
    );
    (void) ipc_send_packet(ipc_conn_fd, headers, headers_len, GG_NULL_READER);

    clear_stream_index(index);
}
