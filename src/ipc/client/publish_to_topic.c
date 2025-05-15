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
#include <string.h>
#include <stdint.h>

static GglError publish_to_topic_common(
    GglBuffer topic, GglMap publish_message
) {
    GglMap args = GGL_MAP(
        ggl_kv(GGL_STR("topic"), ggl_obj_buf(topic)),
        ggl_kv(GGL_STR("publishMessage"), ggl_obj_map(publish_message))
    );

    GglArena error_alloc = ggl_arena_init(GGL_BUF((uint8_t[128]) { 0 }));
    GglIpcError remote_error = GGL_IPC_ERROR_DEFAULT;
    GglError ret = ggipc_call(
        GGL_STR("aws.greengrass#PublishToTopic"),
        GGL_STR("aws.greengrass#PublishToTopicRequest"),
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

GglError ggipc_publish_to_topic_binary(
    GglBuffer topic, GglBuffer payload, GglArena alloc
) {
    GglBuffer encoded_payload;
    GglError ret = ggl_base64_encode(payload, &alloc, &encoded_payload);
    if (ret != GGL_ERR_OK) {
        return ret;
    }
    GglMap binary_message
        = GGL_MAP(ggl_kv(GGL_STR("message"), ggl_obj_buf(encoded_payload)));
    GglMap publish_message
        = GGL_MAP(ggl_kv(GGL_STR("binaryMessage"), ggl_obj_map(binary_message))
        );

    return publish_to_topic_common(topic, publish_message);
}

GglError ggipc_publish_to_topic_obj(GglBuffer topic, GglObject payload) {
    GglMap json_message = GGL_MAP(ggl_kv(GGL_STR("message"), payload));
    GglMap publish_message
        = GGL_MAP(ggl_kv(GGL_STR("jsonMessage"), ggl_obj_map(json_message)));

    return publish_to_topic_common(topic, publish_message);
}
