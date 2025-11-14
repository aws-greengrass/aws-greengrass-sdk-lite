#include "gg/ipc/packet_sequences.h"
#include <gg/ipc/mock.h>
#include <gg/map.h>
#include <gg/object.h>

GgipcPacket gg_test_connect_packet(GgBuffer auth_token) {
    static EventStreamHeader headers[] = {
        { GG_STR(":message-type"),
          { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_CONNECT } },
        { GG_STR(":message-flags"), { EVENTSTREAM_INT32, .int32 = 0 } },
        { GG_STR(":stream-id"), { EVENTSTREAM_INT32, .int32 = 0 } },
        { GG_STR(":version"),
          { EVENTSTREAM_STRING, .string = GG_STR("0.1.0") } },
    };

    static GgKV payload[1];
    payload[0] = gg_kv(GG_STR("authToken"), gg_obj_buf(auth_token));
    size_t payload_len = sizeof(payload) / sizeof(payload[0]);

    uint32_t header_count = sizeof(headers) / sizeof(headers[0]);
    return (GgipcPacket) { .direction = CLIENT_TO_SERVER,
                           .has_payload = true,
                           .payload = gg_obj_map((GgMap) {
                               .pairs = payload, .len = payload_len }),
                           .headers = headers,
                           .header_count = header_count };
}

GgipcPacket gg_test_connect_ack_packet(void) {
    {
        static EventStreamHeader headers[] = {
            { GG_STR(":message-type"),
              { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_CONNECT_ACK } },
            { GG_STR(":message-flags"),
              { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_CONNECTION_ACCEPTED } },
            { GG_STR(":stream-id"), { EVENTSTREAM_INT32, .int32 = 0 } },
        };
        uint32_t header_count = sizeof(headers) / sizeof(headers[0]);
        return (GgipcPacket) { .direction = SERVER_TO_CLIENT,
                               .has_payload = false,
                               .headers = headers,
                               .header_count = header_count };
    }
}

GgipcPacketSequence gg_test_conn_ack_sequence(GgBuffer auth_token) {
    static GgipcPacket packets[2];
    packets[0] = gg_test_connect_packet(auth_token);
    packets[1] = gg_test_connect_ack_packet();
    size_t len = sizeof(packets) / sizeof(packets[0]);
    return (GgipcPacketSequence) { .packets = packets, .len = len };
}

GgipcPacketSequence gg_test_connect_hangup_sequence(GgBuffer auth_token) {
    static GgipcPacket packets[1];
    packets[0] = gg_test_connect_packet(auth_token);
    size_t len = sizeof(packets) / sizeof(packets[0]);
    return (GgipcPacketSequence) { .packets = packets, .len = len };
}
