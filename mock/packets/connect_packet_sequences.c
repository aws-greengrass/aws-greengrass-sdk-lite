
#include "packets.h"
#include <gg/eventstream/rpc.h>
#include <gg/ipc/mock.h>
#include <gg/ipc/packet_sequences.h>
#include <gg/map.h>
#include <stdbool.h>
#include <stddef.h>

GgipcPacket gg_test_connect_packet(GgBuffer auth_token) {
    static GgKV payload[1];
    payload[0] = gg_kv(GG_STR("authToken"), gg_obj_buf(auth_token));
    size_t payload_len = sizeof(payload) / sizeof(payload[0]);

    return (GgipcPacket) {
        .direction = CLIENT_TO_SERVER,
        .has_payload = true,
        .payload = gg_obj_map((GgMap) { .pairs = payload, .len = payload_len }),
        .headers
        = { { GG_STR(":message-type"),
              { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_CONNECT } },
            { GG_STR(":message-flags"), { EVENTSTREAM_INT32, .int32 = 0 } },
            { GG_STR(":stream-id"), { EVENTSTREAM_INT32, .int32 = 0 } },
            { GG_STR(":version"),
              { EVENTSTREAM_STRING, .string = GG_STR("0.1.0") } } },
        .header_count = 4
    };
}

GgipcPacket gg_test_connect_ack_packet(void) {
    {
        return (GgipcPacket) { .direction = SERVER_TO_CLIENT,
                               .has_payload = false,
                               .headers = {
            { GG_STR(":message-type"),
              { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_CONNECT_ACK } },
            { GG_STR(":message-flags"),
              { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_CONNECTION_ACCEPTED } },
            { GG_STR(":stream-id"), { EVENTSTREAM_INT32, .int32 = 0 } }, },
            .header_count = 3 };
    }
}

GgipcPacketSequence gg_test_connect_accepted_sequence(GgBuffer auth_token) {
    return (GgipcPacketSequence) { .packets
                                   = { gg_test_connect_packet(auth_token),
                                       gg_test_connect_ack_packet() },
                                   .len = 2 };
}

GgipcPacketSequence gg_test_connect_hangup_sequence(GgBuffer auth_token) {
    return (GgipcPacketSequence) { .packets
                                   = { gg_test_connect_packet(auth_token) },
                                   .len = 1 };
}
