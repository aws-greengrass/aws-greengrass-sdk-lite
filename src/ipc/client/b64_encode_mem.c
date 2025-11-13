// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/arena.h>
#include <gg/base64.h>
#include <gg/buffer.h>
#include <gg/cleanup.h>
#include <gg/error.h>
#include <gg/ipc/client.h>
#include <gg/ipc/limits.h>
#include <gg/log.h>
#include <inttypes.h>
#include <pthread.h>

static uint8_t ipc_b64_encode_mem[GG_IPC_MAX_MSG_LEN] = { 0 };
static pthread_mutex_t ipc_b64_encode_mtx = PTHREAD_MUTEX_INITIALIZER;

GgError ggipc_publish_to_topic_binary(GgBuffer topic, GgBuffer payload) {
    GG_MTX_SCOPE_GUARD(&ipc_b64_encode_mtx);
    GgArena arena = gg_arena_init(GG_BUF(ipc_b64_encode_mem));

    GgBuffer b64_payload;
    GgError ret = gg_base64_encode(payload, &arena, &b64_payload);
    if (ret != GG_ERR_OK) {
        GG_LOGE(
            "Insufficient memory provided to base64 encode PublishToTopic payload (required %zu, provided %" PRIu32
            ").",
            ((payload.len + 2) / 3) * 4,
            arena.capacity - arena.index
        );
        return ret;
    }

    return ggipc_publish_to_topic_binary_b64(topic, b64_payload);
}

GgError ggipc_publish_to_iot_core(
    GgBuffer topic_name, GgBuffer payload, uint8_t qos
) {
    GG_MTX_SCOPE_GUARD(&ipc_b64_encode_mtx);
    GgArena arena = gg_arena_init(GG_BUF(ipc_b64_encode_mem));

    GgBuffer b64_payload;
    GgError ret = gg_base64_encode(payload, &arena, &b64_payload);
    if (ret != GG_ERR_OK) {
        GG_LOGE(
            "Insufficient memory provided to base64 encode PublishToIoTCore payload (required %zu, available %" PRIu32
            ").",
            ((payload.len + 2) / 3) * 4,
            arena.capacity - arena.index
        );
        return ret;
    }

    return ggipc_publish_to_iot_core_b64(topic_name, b64_payload, qos);
}
