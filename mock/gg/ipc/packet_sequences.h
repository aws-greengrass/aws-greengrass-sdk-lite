#ifndef GG_IPC_PACKET_SEQUENCES_H
#define GG_IPC_PACKET_SEQUENCES_H

#include <gg/ipc/mock.h>
#include <stddef.h>
#include <stdint.h>

/// connect followed by connect ack
GgipcPacketSequence gg_test_connect_accepted_sequence(GgBuffer auth_token);

/// connect with no server response.
GgipcPacketSequence gg_test_connect_hangup_sequence(GgBuffer auth_token);

GgipcPacketSequence gg_test_config_get_object_sequence(
    int32_t stream_id,
    GgBufList key_path,
    GgBuffer *component_name,
    GgObject value
);

GgipcPacketSequence gg_test_mqtt_publish_accepted_sequence(
    int32_t stream_id, GgBuffer topic, GgBuffer payload_base64, GgBuffer qos
);

/// Failed PublishToIotCore request (server responds with generic ServiceError)
GgipcPacketSequence gg_test_mqtt_publish_error_sequence(
    int32_t stream_id, GgBuffer topic, GgBuffer payload_base64, GgBuffer qos
);

GgipcPacketSequence gg_test_mqtt_subscribe_accepted_sequence(
    int32_t stream_id,
    GgBuffer topic,
    GgBuffer payload_base64,
    GgBuffer qos,
    size_t messages
);

#endif
