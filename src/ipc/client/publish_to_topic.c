// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/client.h>
#include <ggl/ipc/client_raw.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <string.h>

static GglError error_handler(
    void *ctx, GglBuffer error_code, GglBuffer message
) {
    (void) ctx;

    GGL_LOGE(
        "Received PublishToTopic error %.*s: %.*s.",
        (int) error_code.len,
        error_code.data,
        (int) message.len,
        message.data
    );

    if (ggl_buffer_eq(error_code, GGL_STR("UnauthorizedError"))) {
        return GGL_ERR_UNSUPPORTED;
    }
    return GGL_ERR_FAILURE;
}

static GglError publish_to_topic_common(
    GglBuffer topic, GglMap publish_message
) {
    GglMap args = GGL_MAP(
        ggl_kv(GGL_STR("topic"), ggl_obj_buf(topic)),
        ggl_kv(GGL_STR("publishMessage"), ggl_obj_map(publish_message))
    );

    return ggipc_call(
        GGL_STR("aws.greengrass#PublishToTopic"),
        GGL_STR("aws.greengrass#PublishToTopicRequest"),
        args,
        NULL,
        &error_handler,
        NULL
    );
}

GglError ggipc_publish_to_topic_json(GglBuffer topic, GglMap payload) {
    GglMap json_message
        = GGL_MAP(ggl_kv(GGL_STR("message"), ggl_obj_map(payload)));
    GglMap publish_message
        = GGL_MAP(ggl_kv(GGL_STR("jsonMessage"), ggl_obj_map(json_message)));

    return publish_to_topic_common(topic, publish_message);
}

GglError ggipc_publish_to_topic_binary_b64(
    GglBuffer topic, GglBuffer b64_payload
) {
    GglMap binary_message
        = GGL_MAP(ggl_kv(GGL_STR("message"), ggl_obj_buf(b64_payload)));
    GglMap publish_message = GGL_MAP(
        ggl_kv(GGL_STR("binaryMessage"), ggl_obj_map(binary_message))
    );

    return publish_to_topic_common(topic, publish_message);
}
