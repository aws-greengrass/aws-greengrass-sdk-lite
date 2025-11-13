#include "gg/ipc/mock.h"
#include "gg/object_compare.h"
#include "inttypes.h"
#include "sys/epoll.h"
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
#include <gg/file.h>
#include <gg/io.h>
#include <gg/ipc/limits.h>
#include <gg/json_decode.h>
#include <gg/json_encode.h>
#include <gg/log.h>
#include <gg/map.h>
#include <gg/object.h>
#include <gg/object_visit.h>
#include <gg/socket.h>
#include <gg/socket_epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#define EVENSTREAM_MAX_HEADER_COUNT 50

static uint8_t ipc_recv_mem[GG_IPC_MAX_MSG_LEN];
static uint8_t ipc_recv_decode_mem[sizeof(GgObject[GG_MAX_OBJECT_SUBOBJECTS])];

static int sock_fd = -1;
static int epoll_fd = -1;
static int client_fd = -1;

static GgError configure_server_socket(
    int socket_fd, GgBuffer path, mode_t mode
) {
    assert(socket_fd >= 0);

    struct sockaddr_un addr = { .sun_family = AF_UNIX, .sun_path = { 0 } };

    // TODO: Handle long paths by creating socket in temp dir and moving
    if (path.len >= sizeof(addr.sun_path)) {
        GG_LOGE(
            "Socket path too long (len %zu, max %zu).",
            path.len,
            sizeof(addr.sun_path) - 1
        );
        return GG_ERR_FAILURE;
    }

    memcpy(addr.sun_path, path.data, path.len);

    if ((unlink(addr.sun_path) == -1) && (errno != ENOENT)) {
        GG_LOGE("Failed to unlink server socket: %d.", errno);
        return GG_ERR_FAILURE;
    }

    if (bind(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        GG_LOGE("Failed to bind server socket: %d.", errno);
        return GG_ERR_FAILURE;
    }

    if (chmod(addr.sun_path, mode) == -1) {
        GG_LOGE("Failed to chmod server socket: %d.", errno);
        return GG_ERR_FAILURE;
    }

    // To prevent deadlocking on hanged client, add a timeout
    struct timeval timeout = { .tv_sec = 5 };
    int sys_ret = setsockopt(
        socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)
    );
    if (sys_ret == -1) {
        GG_LOGE("Failed to set receive timeout on socket: %d.", errno);
        return GG_ERR_FATAL;
    }
    sys_ret = setsockopt(
        socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)
    );
    if (sys_ret == -1) {
        GG_LOGE("Failed to set send timeout on socket: %d.", errno);
        return GG_ERR_FATAL;
    }

    // Client must only attempt to connect once during tests
    static const int MAX_SOCKET_BACKLOG = 1;
    if (listen(socket_fd, MAX_SOCKET_BACKLOG) == -1) {
        GG_LOGE("Failed to listen on server socket: %d.", errno);
        return GG_ERR_FAILURE;
    }

    return GG_ERR_OK;
}

static GgError gg_socket_open(GgBuffer path, mode_t mode, int *socket_fd) {
    assert(socket_fd != NULL);
    int server_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (server_fd == -1) {
        GG_LOGE("Failed to create socket: %d.", errno);
        return GG_ERR_FAILURE;
    }

    GgError ret = configure_server_socket(server_fd, path, mode);
    if (ret != GG_ERR_OK) {
        cleanup_close(&server_fd);
        return ret;
    }
    *socket_fd = server_fd;
    return ret;
}

GgError gg_test_setup_ipc(
    const char *path, mode_t mode, int *handle, const char *auth_token
) {
    int socket_fd = -1;
    GgError ret = gg_socket_open(
        gg_buffer_from_null_term((char *) path), mode, &socket_fd
    );
    if (ret != GG_ERR_OK) {
        return ret;
    }
    GG_CLEANUP_ID(cleanup_sock, cleanup_close, socket_fd);

    ret = gg_socket_epoll_create(&epoll_fd);
    if (ret != GG_ERR_OK) {
        epoll_fd = -1;
        return ret;
    }
    GG_CLEANUP_ID(cleanup_epollfd, cleanup_close, epoll_fd);
    ret = gg_socket_epoll_add(epoll_fd, socket_fd, UINT64_MAX);
    if (ret != GG_ERR_OK) {
        return ret;
    }

    cleanup_sock = -1;
    cleanup_epollfd = -1;
    sock_fd = socket_fd;

    *handle = 1;

    // test setup code should be called before creating any threads
    // NOLINTBEGIN(concurrency-mt-unsafe)
    int setenv_ret = setenv(
        "AWS_GG_NUCLEUS_DOMAIN_SOCKET_FILEPATH_FOR_COMPONENT", path, true
    );
    if (setenv_ret != 0) {
        GG_LOGE("Failed to set socket environment variable: %d.", errno);
        return GG_ERR_FAILURE;
    }

    setenv_ret = setenv("SVCUID", auth_token, true);
    if (setenv_ret != 0) {
        GG_LOGE("Failed to set auth token environment variable: %d.", errno);
        return GG_ERR_FAILURE;
    }
    // NOLINTEND(concurrency-mt-unsafe)

    return GG_ERR_OK;
}

static GgError gg_test_send_packet(const GgipcPacket *packet, int client) {
    GgBuffer packet_bytes = GG_BUF(ipc_recv_mem);
    GgError ret = eventstream_encode(
        &packet_bytes,
        packet->headers,
        packet->header_count,
        packet->has_payload ? gg_json_reader(&packet->payload) : GG_NULL_READER
    );
    if (ret != GG_ERR_OK) {
        return ret;
    }
    return gg_socket_write(client, packet_bytes);
}

static GgError find_header_int(
    EventStreamHeaderIter headers, GgBuffer name, int32_t value
) {
    EventStreamHeaderIter iter = headers;
    EventStreamHeader header;
    while (eventstream_header_next(&iter, &header) == GG_ERR_OK) {
        if (gg_buffer_eq(header.name, name)) {
            if (header.value.type != EVENTSTREAM_INT32) {
                GG_LOGE("Expected header value to be int.");
                break;
            }
            if (header.value.int32 != value) {
                GG_LOGE(
                    "Expected header value %" PRIi32 ", got %" PRIi32 ".",
                    value,
                    header.value.int32
                );
                break;
            }
            return GG_ERR_OK;
        }
    }
    GG_LOGE("Expected header %.*s not found.", (int) name.len, name.data);
    return GG_ERR_FAILURE;
}

static GgError find_header_str(
    EventStreamHeaderIter headers, GgBuffer name, GgBuffer value
) {
    EventStreamHeaderIter iter = headers;
    EventStreamHeader header;
    while (eventstream_header_next(&iter, &header) == GG_ERR_OK) {
        if (gg_buffer_eq(header.name, name)) {
            if (header.value.type != EVENTSTREAM_STRING) {
                GG_LOGE("Expected header value to be string.");
                break;
            }
            if (!gg_buffer_eq(value, header.value.string)) {
                GG_LOGE(
                    "Expected header value %.*s, got %.*s.",
                    (int) value.len,
                    value.data,
                    (int) header.value.string.len,
                    header.value.string.data
                );
                break;
            }
            return GG_ERR_OK;
        }
    }
    GG_LOGE("Expected header \"%.*s\" not found.", (int) name.len, name.data);
    return GG_ERR_FAILURE;
}

static GgError gg_file_writer_write(void *ctx, GgBuffer buf) {
    int *fd = ctx;
    return gg_file_write(*fd, buf);
}

static GgWriter gg_file_writer(int *fd) {
    return (GgWriter) { .ctx = fd, .write = gg_file_writer_write };
}

static void print_client_packet(EventStreamMessage msg) {
    GG_LOGE("Client headers:");

    EventStreamHeaderIter iter = msg.headers;
    EventStreamHeader header;
    while (eventstream_header_next(&iter, &header) == GG_ERR_OK) {
        switch (header.value.type) {
        case EVENTSTREAM_INT32:
            GG_LOGE(
                "%.*s=%" PRIi32,
                (int) header.name.len,
                header.name.data,
                header.value.int32
            );
            break;
        case EVENTSTREAM_STRING:
            GG_LOGE(
                "%.*s=%.*s",
                (int) header.name.len,
                header.name.data,
                (int) header.value.string.len,
                header.value.string.data
            );
            break;
        default:
            break;
        }
    }
}

static GgError gg_test_recv_packet(const GgipcPacket *packet, int client) {
    GgBuffer packet_bytes = GG_BUF(ipc_recv_mem);
    EventStreamMessage msg = { 0 };
    GgError ret
        = eventsteam_get_packet(gg_socket_reader(&client), &msg, packet_bytes);
    if (ret != GG_ERR_OK) {
        return ret;
    }

    // check headers
    if (packet->header_count != msg.headers.count) {
        GG_LOGE(
            "Expected header count %" PRIu32 ", got %" PRIu32 ".",
            packet->header_count,
            msg.headers.count
        );
    }

    for (EventStreamHeader *header = packet->headers;
         header < &packet->headers[packet->header_count];
         ++header) {
        GgBuffer key = header->name;
        switch (header->value.type) {
        case EVENTSTREAM_INT32: {
            ret = find_header_int(msg.headers, key, header->value.int32);
            if (ret != GG_ERR_OK) {
                print_client_packet(msg);
                return ret;
            }
            break;
        }
        case EVENTSTREAM_STRING: {
            ret = find_header_str(msg.headers, key, header->value.string);
            if (ret != GG_ERR_OK) {
                print_client_packet(msg);
                return ret;
            }
            break;
        }
        default:
            GG_LOGE(
                "INTERNAL TEST ERROR: Unhandled header type %d.",
                header->value.type
            );
            assert(false);
            return GG_ERR_FAILURE;
        }
    }

    if (!packet->has_payload) {
        return GG_ERR_OK;
    }

    GgArena json_arena = gg_arena_init(GG_BUF(ipc_recv_decode_mem));
    GgObject payload_obj = GG_OBJ_NULL;
    ret = gg_json_decode_destructive(msg.payload, &json_arena, &payload_obj);
    if (ret != GG_ERR_OK) {
        return ret;
    }
    // Compare GgObject representation of payload
    if (gg_obj_type(payload_obj) != GG_TYPE_MAP) {
        GG_LOGE("Expected payload to be map");
        return GG_ERR_FAILURE;
    }

    if (!gg_obj_eq(packet->payload, payload_obj)) {
        int stderr_fd = STDERR_FILENO;

        GG_LOGE("Expected payload mismatch");
        GG_LOGE("Expected payload:");
        (void) gg_json_encode(packet->payload, gg_file_writer(&stderr_fd));
        GG_LOGE("Actual payload:");
        (void) gg_json_encode(payload_obj, gg_file_writer(&stderr_fd));
        return GG_ERR_FAILURE;
    }

    return GG_ERR_OK;
}

GgError gg_test_expect_packet_sequence(
    GgipcPacketSequence sequence, int client_timeout, int handle
) {
    assert(handle > 0);
    assert(sock_fd >= 0);
    assert(epoll_fd >= 0);

    if (client_timeout < 5) {
        client_timeout = 5;
    }

    struct epoll_event events[1];
    int max_events = sizeof(events) / sizeof(events[0]);

    GG_LOGD("Waiting for client to connect.");
    int epoll_ret;
    do {
        epoll_ret
            = epoll_wait(epoll_fd, events, max_events, client_timeout * 1000);
    } while ((epoll_ret == -1) && (errno == EINTR));
    if (epoll_ret == -1) {
        GG_LOGE("Failed to wait for test client connect (%d).", errno);
        return GG_ERR_TIMEOUT;
    }
    struct sockaddr_un addr = { .sun_family = AF_UNIX, .sun_path = { 0 } };
    socklen_t len = sizeof(addr);
    client_fd = accept(sock_fd, &addr, &len);
    if (client_fd < 0) {
        GG_LOGE("Failed to accept test client.");
        return GG_ERR_TIMEOUT;
    }
    // To prevent deadlocking on hanged client, add a timeout
    struct timeval timeout = { .tv_sec = client_timeout };
    int sys_ret = setsockopt(
        client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)
    );
    if (sys_ret == -1) {
        GG_LOGE("Failed to set receive timeout on socket: %d.", errno);
        return GG_ERR_FATAL;
    }
    sys_ret = setsockopt(
        client_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)
    );
    if (sys_ret == -1) {
        GG_LOGE("Failed to set send timeout on socket: %d.", errno);
        return GG_ERR_FATAL;
    }

    GG_LOGD("Awaiting sequence of %zu packets.", sequence.len);
    GG_PACKET_SEQUENCE_FOREACH(packet, sequence) {
        if (packet->direction == 0) {
            GgError ret = gg_test_recv_packet(packet, client_fd);
            if (ret != GG_ERR_OK) {
                GG_LOGE("Failed to receive a packet.");
                return ret;
            }
        } else {
            GgError ret = gg_test_send_packet(packet, client_fd);
            if (ret != GG_ERR_OK) {
                GG_LOGE("Failed to send a packet.");
                return ret;
            }
        }
    }

    return GG_ERR_OK;
}

GgError gg_test_disconnect(int handle) {
    assert(handle > 0);
    if (client_fd < 0) {
        GG_LOGE("Client not connected.");
        return GG_ERR_NOENTRY;
    }

    int old_client_fd = client_fd;
    client_fd = -1;
    return gg_close(old_client_fd);
}

void gg_test_close(int handle) {
    assert(handle > 0);
    if (client_fd >= 0) {
        (void) gg_close(client_fd);
        client_fd = -1;
    }
    if (epoll_fd >= 0) {
        (void) gg_close(epoll_fd);
        epoll_fd = -1;
    }
    if (sock_fd >= 0) {
        (void) gg_close(sock_fd);
        sock_fd = -1;
    }
}
