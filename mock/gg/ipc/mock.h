#ifndef GG_IPC_MOCK_H
#define GG_IPC_MOCK_H

#include <gg/attr.h>
#include <sys/types.h>
#include <stddef.h>

#ifdef __cplusplus
#include <gg/ipc/mock_types.hpp>
#include <gg/types.hpp>
#else
#include <gg/eventstream/rpc.h>
#include <gg/eventstream/types.h>
#include <gg/object.h>
#endif

typedef enum {
    CLIENT_TO_SERVER = 0,
    SERVER_TO_CLIENT = 1
} GgipcPacketDirection;

/// Sets up the IPC mock for this thread, opening a socket in a directory
/// containing the socket path and setting appropriate IPC environment
/// variables. The IPC mock will not accept connections until
/// gg_test_expect_packet_sequence() is called.
NULL_TERMINATED_STRING_ARG(1) NULL_TERMINATED_STRING_ARG(3)
GgError gg_test_setup_ipc(
    const char *socket_path_prefix, mode_t mode, const char *auth_token
);

/// Retrieves the socket path created by the IPC mock.
GgBuffer gg_test_get_socket_path(void);
/// Retrieves the auth token used to init the IPC mock.
GgBuffer gg_test_get_auth_token(void);

#define GG_TEST_MAX_HEADER_COUNT 10

typedef struct {
    EventStreamHeader headers[GG_TEST_MAX_HEADER_COUNT];
    uint32_t header_count;
    /// 0 = client->server, 1 = server->client
    int direction;
    /// empty payload "{}" is semantically-different from no payload "".
    bool has_payload;
    GgObject payload;
} GgipcPacket;

typedef struct {
    GgipcPacket packets[10];
    size_t len;
} GgipcPacketSequence;

#define GG_PACKET_SEQUENCE(...) \
    (GgipcPacketSequence) { \
        .packets = (GgipcPacket[]) { __VA_ARGS__ }, \
        .len = (sizeof((GgipcPacket[]) { __VA_ARGS__ })) \
            / (sizeof(GgipcPacket)) \
    }

// NOLINTBEGIN(bugprone-macro-parentheses)
/// Loop over the packets in a packet sequence.
#define GG_PACKET_SEQUENCE_FOREACH(name, seq) \
    for (GgipcPacket *name = (seq).packets; name < &(seq).packets[(seq).len]; \
         name = &name[1])

// NOLINTEND(bugprone-macro-parentheses)

/// Blocks until client process connects.
/// The client timeout is the time in seconds to wait for
GgError gg_test_accept_client(int client_timeout);

/// Blocks until all packets in the packet sequence are validated or until any
/// are invalidated. Returns GG_ERR_OK if all packets were successfully
/// sent/received. The client timeout is the time in seconds to wait for the
/// next client packet before failing with GG_ERR_TIMEOUT.
GgError gg_test_expect_packet_sequence(
    GgipcPacketSequence sequence, int client_timeout
);

/// Hangs up on the client
GgError gg_test_disconnect(void);

/// Blocks until client closes socket. If any data is received on the socket, or
/// if the socket is not closed inside the timeout, returns a failure.
GgError gg_test_wait_for_client_disconnect(int client_timeout);

/// Closes all file descriptors opened by the mock without waiting for client
/// disconnect.
void gg_test_close(void);

#endif
