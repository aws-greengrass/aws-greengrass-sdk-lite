// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/ipc/client.h>
#include <gg/ipc/client_raw.h>
#include <gg/log.h>
#include <gg/map.h>
#include <gg/object.h>
#include <string.h>

static GgError error_handler(void *ctx, GgBuffer error_code, GgBuffer message) {
    (void) ctx;

    GG_LOGE(
        "Received PublishToTopic error %.*s: %.*s.",
        (int) error_code.len,
        error_code.data,
        (int) message.len,
        message.data
    );

    if (gg_buffer_eq(error_code, GG_STR("UnauthorizedError"))) {
        return GG_ERR_UNSUPPORTED;
    }
    return GG_ERR_FAILURE;
}

static GgError publish_to_topic_common(GgBuffer topic, GgMap publish_message) {
    GgMap args = GG_MAP(
        gg_kv(GG_STR("topic"), gg_obj_buf(topic)),
        gg_kv(GG_STR("publishMessage"), gg_obj_map(publish_message))
    );

    return ggipc_call(
        GG_STR("aws.greengrass#PublishToTopic"),
        GG_STR("aws.greengrass#PublishToTopicRequest"),
        args,
        NULL,
        &error_handler,
        NULL
    );
}

GgError ggipc_publish_to_topic_json(GgBuffer topic, GgMap payload) {
    GgMap json_message = GG_MAP(gg_kv(GG_STR("message"), gg_obj_map(payload)));
    GgMap publish_message
        = GG_MAP(gg_kv(GG_STR("jsonMessage"), gg_obj_map(json_message)));

    return publish_to_topic_common(topic, publish_message);
}

GgError ggipc_publish_to_topic_binary_b64(
    GgBuffer topic, GgBuffer b64_payload
) {
    GgMap binary_message
        = GG_MAP(gg_kv(GG_STR("message"), gg_obj_buf(b64_payload)));
    GgMap publish_message
        = GG_MAP(gg_kv(GG_STR("binaryMessage"), gg_obj_map(binary_message)));

    return publish_to_topic_common(topic, publish_message);
}
