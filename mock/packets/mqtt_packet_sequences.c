
#include "gg/ipc/packet_sequences.h"
#include "packets.h"
#include <assert.h>
#include <gg/ipc/mock.h>
#include <gg/map.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

GgipcPacket gg_test_mqtt_publish_request_packet(
    int32_t stream_id, GgBuffer topic, GgBuffer payload_base64, GgBuffer qos
) {
    static GgKV pairs[3];
    pairs[0] = gg_kv(GG_STR("topicName"), gg_obj_buf(topic));
    pairs[1] = gg_kv(GG_STR("payload"), gg_obj_buf(payload_base64));
    pairs[2] = gg_kv(GG_STR("qos"), gg_obj_buf(qos));

    size_t pairs_len = sizeof(pairs) / sizeof(pairs[0]);

    return (GgipcPacket) {
        .direction = CLIENT_TO_SERVER,
        .has_payload = true,
        .payload = gg_obj_map((GgMap) { .pairs = pairs, .len = pairs_len }),
        .headers
        = GG_IPC_REQUEST_HEADERS(stream_id, "aws.greengrass#PublishToIoTCore"),
        .header_count = GG_IPC_REQUEST_HEADERS_COUNT
    };
}

GgipcPacket gg_test_mqtt_publish_accepted_packet(int32_t stream_id) {
    return (GgipcPacket) { .direction = SERVER_TO_CLIENT,
                           .has_payload = false,
                           .headers = GG_IPC_ACCEPTED_HEADERS(
                               stream_id, "aws.greengrass#PublishToIoTCore"
                           ),
                           .header_count = GG_IPC_ACCEPTED_HEADERS_COUNT };
}

GgipcPacketSequence gg_test_mqtt_publish_accepted_sequence(
    int32_t stream_id, GgBuffer topic, GgBuffer payload_base64, GgBuffer qos
) {
    return (GgipcPacketSequence) {
        .packets = { gg_test_mqtt_publish_request_packet(
                         stream_id, topic, payload_base64, qos
                     ),
                     gg_test_mqtt_publish_accepted_packet(stream_id) },
        .len = 2
    };
}

GgipcPacketSequence gg_test_mqtt_publish_error_sequence(
    int32_t stream_id, GgBuffer topic, GgBuffer payload_base64, GgBuffer qos
) {
    return (GgipcPacketSequence) {
        .packets = { gg_test_mqtt_publish_request_packet(
                         stream_id, topic, payload_base64, qos
                     ),
                     gg_test_ipc_service_error_packet(stream_id) },
        .len = 2
    };
}

GgipcPacket gg_test_mqtt_message_packet(
    int32_t stream_id, GgBuffer topic, GgBuffer payload_base64
) {
    static GgKV message_pairs[2];
    message_pairs[0] = gg_kv(GG_STR("topicName"), gg_obj_buf(topic));
    message_pairs[1] = gg_kv(GG_STR("payload"), gg_obj_buf(payload_base64));
    size_t inner_pairs_len = sizeof(message_pairs) / sizeof(message_pairs[0]);

    static GgKV payload_pairs[1];
    payload_pairs[0] = gg_kv(
        GG_STR("message"),
        gg_obj_map((GgMap) { .pairs = message_pairs, .len = inner_pairs_len })
    );
    size_t outer_pairs_len = sizeof(payload_pairs) / sizeof(payload_pairs[0]);

    return (GgipcPacket) {
        .direction = SERVER_TO_CLIENT,
        .has_payload = true,
        .payload = gg_obj_map((GgMap) { .pairs = payload_pairs,
                                        .len = outer_pairs_len }),
        .headers = GG_IPC_SUBSCRIBE_MESSAGE_HEADERS(
            stream_id, "aws.greengrass#IoTCoreMessage"
        ),
        .header_count = GG_IPC_SUBSCRIBE_MESSAGE_HEADERS_COUNT,
    };
}

GgipcPacket gg_test_mqtt_subscribe_request_packet(
    int32_t stream_id, GgBuffer topic, GgBuffer qos
) {
    static GgKV pairs[2];
    pairs[0] = gg_kv(GG_STR("topicName"), gg_obj_buf(topic));
    pairs[1] = gg_kv(GG_STR("qos"), gg_obj_buf(qos));
    size_t pairs_len = sizeof(pairs) / sizeof(pairs[0]);

    return (GgipcPacket) { .direction = CLIENT_TO_SERVER,
                           .has_payload = true,
                           .payload = gg_obj_map((GgMap) { .pairs = pairs,
                                                           .len = pairs_len }),
                           .headers = GG_IPC_REQUEST_HEADERS(
                               stream_id, "aws.greengrass#SubscribeToIoTCore"
                           ),
                           .header_count = GG_IPC_REQUEST_HEADERS_COUNT };
}

GgipcPacket gg_test_mqtt_subscribe_accepted_packet(int32_t stream_id) {
    return (GgipcPacket) {
        .direction = SERVER_TO_CLIENT,
        .has_payload = false,
        .headers = GG_IPC_SUBSCRIBE_MESSAGE_HEADERS(
            stream_id, "aws.greengrass#SubscribeToIoTCoreAccepted"
        ),
        .header_count = GG_IPC_SUBSCRIBE_MESSAGE_HEADERS_COUNT
    };
}

GgipcPacketSequence gg_test_mqtt_subscribe_accepted_sequence(
    int32_t stream_id,
    GgBuffer topic,
    GgBuffer payload_base64,
    GgBuffer qos,
    size_t messages
) {
    GgipcPacket response_packet
        = gg_test_mqtt_message_packet(stream_id, topic, payload_base64);

    GgipcPacketSequence seq
        = { .packets
            = { gg_test_mqtt_subscribe_request_packet(stream_id, topic, qos),
                gg_test_mqtt_subscribe_accepted_packet(stream_id) },
            .len = 2 };

    size_t max_len = (sizeof(seq.packets) / sizeof(seq.packets[0]));
    assert(messages <= (max_len - seq.len));

    for (uint32_t i = 0; (i != messages) && (seq.len != max_len);
         ++i, ++seq.len) {
        seq.packets[seq.len] = response_packet;
    }

    return seq;
}
