#ifndef GG_TEST_PACKETS_H
#define GG_TEST_PACKETS_H

#include <gg/arena.h>
#include <gg/eventstream/rpc.h> // IWYU pragma: keep
#include <gg/ipc/mock.h>
#include <gg/log.h>
#include <stdint.h>
#include <stdlib.h>

#define GG_IPC_REQUEST_HEADERS(stream_id, operation) \
    { \
        { GG_STR(":message-type"), \
          { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_APPLICATION_MESSAGE } }, \
            { GG_STR(":message-flags"), { EVENTSTREAM_INT32, .int32 = 0 } }, \
            { GG_STR(":stream-id"), \
              { EVENTSTREAM_INT32, .int32 = (stream_id) } }, \
            { GG_STR("operation"), \
              { EVENTSTREAM_STRING, .string = GG_STR(operation) } }, \
        { \
            GG_STR("service-model-type"), { \
                EVENTSTREAM_STRING, .string = GG_STR(operation "Request") \
            } \
        } \
    }

#define GG_IPC_REQUEST_HEADERS_COUNT 5

#define GG_IPC_ACCEPTED_HEADERS(stream_id, operation) \
    { \
        { GG_STR(":message-type"), \
          { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_APPLICATION_MESSAGE } }, \
            { GG_STR(":message-flags"), \
              { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_TERMINATE_STREAM } }, \
            { GG_STR(":stream-id"), \
              { EVENTSTREAM_INT32, .int32 = (stream_id) } }, \
            { GG_STR(":content-type"), \
              { EVENTSTREAM_STRING, .string = GG_STR("application/json") } }, \
        { \
            GG_STR("service-model-type"), { \
                EVENTSTREAM_STRING, .string = GG_STR(operation "Accepted") \
            } \
        } \
    }

#define GG_IPC_ACCEPTED_HEADERS_COUNT 5

#define GG_IPC_SUBSCRIBE_MESSAGE_HEADERS(stream_id, service_model) \
    { \
        { GG_STR(":message-type"), \
          { EVENTSTREAM_INT32, .int32 = EVENTSTREAM_APPLICATION_MESSAGE } }, \
            { GG_STR(":message-flags"), { EVENTSTREAM_INT32, .int32 = 0 } }, \
            { GG_STR(":stream-id"), \
              { EVENTSTREAM_INT32, .int32 = (stream_id) } }, \
            { GG_STR(":content-type"), \
              { EVENTSTREAM_STRING, .string = GG_STR("application/json") } }, \
        { \
            GG_STR("service-model-type"), { \
                EVENTSTREAM_STRING, .string = GG_STR(service_model) \
            } \
        } \
    }

#define GG_IPC_SUBSCRIBE_MESSAGE_HEADERS_COUNT 5

/// A packet sequence which uses multiple of the same packet type with
/// differences in payload must copy the packet to an arena before including
/// it in the sequence.
static inline GgipcPacket gg_test_claim_packet(
    GgArena *arena,
    GgipcPacket packet,
    const char *file,
    int line,
    const char *tag
) {
    GgError ret = gg_arena_claim_obj_bufs(&packet.payload, arena);
    if (ret != GG_ERR_OK) {
#if (GG_LOG_LEVEL >= GG_LOG_ERROR)
        gg_log(
            GG_LOG_ERROR, file, line, tag, "Arena too small to alloc packet!"
        );
#endif
        _Exit(1);
    }
    return packet;
}

#define GG_TEST_CLAIM_PACKET(p_arena, packet) \
    gg_test_claim_packet(p_arena, packet, __FILE__, __LINE__, (GG_MODULE))

/// client->server auth token handshake
GgipcPacket gg_test_connect_packet(GgBuffer auth_token);

/// server->client ack of successful connection
GgipcPacket gg_test_connect_ack_packet(void);

/// If component_name not set, not present in request packet
GgipcPacket gg_test_config_get_object_request_packet(
    int32_t stream_id, GgBufList key_path, GgBuffer *component_name
);

/// If component_name not set, a default is used instead
GgipcPacket gg_test_config_get_object_accepted_packet(
    int32_t stream_id,
    GgBuffer *component_name,
    GgBuffer *last_key,
    GgObject value
);

/// server->client generic ServiceError response
GgipcPacket gg_test_ipc_service_error_packet(int32_t stream_id);

/// client->server PublishToIotCore request
GgipcPacket gg_test_mqtt_publish_request_packet(
    int32_t stream_id, GgBuffer topic, GgBuffer payload_base64, GgBuffer qos
);

/// server->client successful PublishToIotCore response
GgipcPacket gg_test_mqtt_publish_accepted_packet(int32_t stream_id);

GgipcPacket gg_test_mqtt_subscribe_request_packet(
    int32_t stream_id, GgBuffer topic, GgBuffer qos
);

GgipcPacket gg_test_mqtt_subscribe_accepted_packet(int32_t stream_id);

GgipcPacket gg_test_mqtt_message_packet(
    int32_t stream_id, GgBuffer topic, GgBuffer payload_base64
);

#endif
