// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/arena.h>
#include <ggl/base64.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/client.h>
#include <ggl/ipc/client_raw.h>
#include <ggl/ipc/error.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <inttypes.h>
#include <string.h>

// TODO: use GglByteVec for payload to allow in-place base64 encoding.
GglError ggipc_publish_to_iot_core(
    int conn,
    GglBuffer topic_name,
    GglBuffer payload,
    uint8_t qos,
    GglArena *alloc
) {
    if (qos > 2) {
        GGL_LOGE("Invalid QoS \"%" PRIu8 "\" provided. QoS must be <= 2", qos);
        return GGL_ERR_INVALID;
    }
    GGL_LOGT("Topic name len: %zu", topic_name.len);
    GglBuffer qos_buffer = GGL_BUF((uint8_t[1]) { qos + (uint8_t) '0' });
    GglBuffer encoded_payload;
    GglError ret = ggl_base64_encode(payload, alloc, &encoded_payload);
    if (ret != GGL_ERR_OK) {
        return ret;
    }
    GglMap args = GGL_MAP(
        ggl_kv(GGL_STR("topicName"), ggl_obj_buf(topic_name)),
        ggl_kv(GGL_STR("payload"), ggl_obj_buf(encoded_payload)),
        ggl_kv(GGL_STR("qos"), ggl_obj_buf(qos_buffer))
    );

    GglArena error_alloc = ggl_arena_init(GGL_BUF((uint8_t[128]) { 0 }));
    GglIpcError remote_error = GGL_IPC_ERROR_DEFAULT;
    ret = ggipc_call(
        conn,
        GGL_STR("aws.greengrass#PublishToIoTCore"),
        GGL_STR("aws.greengrass#PublishToIoTCoreRequest"),
        args,
        &error_alloc,
        NULL,
        &remote_error
    );
    if (ret == GGL_ERR_REMOTE) {
        if (remote_error.error_code == GGL_IPC_ERR_UNAUTHORIZED_ERROR) {
            GGL_LOGE(
                "Component unauthorized: %.*s",
                (int) remote_error.message.len,
                remote_error.message.data
            );
            return GGL_ERR_UNSUPPORTED;
        }
        GGL_LOGE("Server error.");
        return GGL_ERR_FAILURE;
    }

    return ret;
}
