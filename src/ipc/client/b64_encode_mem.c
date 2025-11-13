// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/arena.h>
#include <ggl/base64.h>
#include <ggl/buffer.h>
#include <ggl/cleanup.h>
#include <ggl/error.h>
#include <ggl/ipc/client.h>
#include <ggl/ipc/limits.h>
#include <ggl/log.h>
#include <inttypes.h>
#include <pthread.h>

static uint8_t ipc_b64_encode_mem[GGL_IPC_MAX_MSG_LEN] = { 0 };
static pthread_mutex_t ipc_b64_encode_mtx = PTHREAD_MUTEX_INITIALIZER;

GglError ggipc_publish_to_topic_binary(GglBuffer topic, GglBuffer payload) {
    GGL_MTX_SCOPE_GUARD(&ipc_b64_encode_mtx);
    GglArena arena = ggl_arena_init(GGL_BUF(ipc_b64_encode_mem));

    GglBuffer b64_payload;
    GglError ret = ggl_base64_encode(payload, &arena, &b64_payload);
    if (ret != GGL_ERR_OK) {
        GGL_LOGE(
            "Insufficient memory provided to base64 encode PublishToTopic payload (required %zu, provided %" PRIu32
            ").",
            ((payload.len + 2) / 3) * 4,
            arena.capacity - arena.index
        );
        return ret;
    }

    return ggipc_publish_to_topic_binary_b64(topic, b64_payload);
}

GglError ggipc_publish_to_iot_core(
    GglBuffer topic_name, GglBuffer payload, uint8_t qos
) {
    GGL_MTX_SCOPE_GUARD(&ipc_b64_encode_mtx);
    GglArena arena = ggl_arena_init(GGL_BUF(ipc_b64_encode_mem));

    GglBuffer b64_payload;
    GglError ret = ggl_base64_encode(payload, &arena, &b64_payload);
    if (ret != GGL_ERR_OK) {
        GGL_LOGE(
            "Insufficient memory provided to base64 encode PublishToIoTCore payload (required %zu, available %" PRIu32
            ").",
            ((payload.len + 2) / 3) * 4,
            arena.capacity - arena.index
        );
        return ret;
    }

    return ggipc_publish_to_iot_core_b64(topic_name, b64_payload, qos);
}
