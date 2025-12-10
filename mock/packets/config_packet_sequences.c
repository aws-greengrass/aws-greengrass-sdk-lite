#include "gg/ipc/mock.h"
#include "gg/ipc/packet_sequences.h"
#include "packets/packets.h"
#include "stdlib.h"
#include <assert.h>
#include <gg/buffer.h>
#include <gg/map.h>
#include <gg/object.h>
#include <stdbool.h>
#include <stdio.h>

GgipcPacket gg_test_config_get_object_request_packet(
    int32_t stream_id, GgBufList key_path, GgBuffer *component_name
) {
    static GgObject list[10];
    assert(sizeof(list) / sizeof(list[0]) >= key_path.len);
    for (size_t i = 0; i != key_path.len; ++i) {
        list[i] = gg_obj_buf(key_path.bufs[i]);
    }
    size_t list_len = key_path.len;

    static GgKV pairs[2];
    pairs[0] = gg_kv(
        GG_STR("keyPath"),
        gg_obj_list((GgList) { .items = list, .len = list_len })
    );
    if (component_name != NULL) {
        pairs[1] = gg_kv(GG_STR("componentName"), gg_obj_buf(*component_name));
    }
    size_t pairs_len = sizeof(pairs) / sizeof(pairs[0]);
    if (component_name == NULL) {
        pairs_len--;
    }

    return (GgipcPacket) {
        .direction = CLIENT_TO_SERVER,
        .has_payload = true,
        .payload = gg_obj_map((GgMap) { .pairs = pairs, .len = pairs_len }),
        .headers
        = GG_IPC_REQUEST_HEADERS(stream_id, "aws.greengrass#GetConfiguration"),
        .header_count = GG_IPC_REQUEST_HEADERS_COUNT
    };
}

GgipcPacket gg_test_config_get_object_accepted_packet(
    int32_t stream_id,
    GgBuffer *component_name,
    GgBuffer *last_key,
    GgObject value
) {
    static GgKV payload[2];
    if (last_key == NULL) {
        payload[0] = gg_kv(GG_STR("value"), value);
    } else {
        static GgKV payload_inner[1];
        payload_inner[0] = gg_kv(*last_key, value);
        size_t payload_inner_len
            = sizeof(payload_inner) / sizeof(payload_inner[0]);
        payload[0] = gg_kv(
            GG_STR("value"),
            gg_obj_map((GgMap) { .pairs = payload_inner,
                                 .len = payload_inner_len })
        );
    }
    payload[1] = gg_kv(
        GG_STR("componentName"),
        gg_obj_buf(
            component_name == NULL ? GG_STR("MyComponent") : *component_name
        )
    );
    size_t payload_len = sizeof(payload) / sizeof(payload[0]);

    return (GgipcPacket) {
        .direction = SERVER_TO_CLIENT,
        .has_payload = true,
        .payload = gg_obj_map((GgMap) { .pairs = payload, .len = payload_len }),
        .headers
        = GG_IPC_ACCEPTED_HEADERS(stream_id, "aws.greengrass#GetConfiguration"),
        .header_count = GG_IPC_ACCEPTED_HEADERS_COUNT
    };
}

GgipcPacketSequence gg_test_config_get_object_sequence(
    int32_t stream_id,
    GgBufList key_path,
    GgBuffer *component_name,
    GgObject value
) {
    GgipcPacket request = gg_test_config_get_object_request_packet(
        stream_id, key_path, component_name
    );

    GgBuffer *last_key = NULL;
    if (key_path.len > 0) {
        last_key = &key_path.bufs[key_path.len - 1];
    }

    GgipcPacket response = gg_test_config_get_object_accepted_packet(
        stream_id, component_name, last_key, value
    );
    return (GgipcPacketSequence) { .packets = { request, response }, .len = 2 };
}
