// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/base64.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/flags.h>
#include <ggl/ipc/client.h>
#include <ggl/ipc/client_raw.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <string.h>
#include <stdbool.h>

static GglError subscribe_to_topic_resp_handler(
    void *ctx,
    GgIpcSubscriptionHandle handle,
    GglBuffer service_model_type,
    GglMap data
) {
    GgIpcSubscribeToTopicCallback *callback = ctx;

    if (!ggl_buffer_eq(
            service_model_type,
            GGL_STR("aws.greengrass#SubscriptionResponseMessage")
        )) {
        GGL_LOGE("Unexpected service-model-type received.");
        return GGL_ERR_INVALID;
    }

    GglObject *json_message_obj;
    GglObject *binary_message_obj;
    GglError ret = ggl_map_validate(
        data,
        GGL_MAP_SCHEMA(
            { GGL_STR("jsonMessage"),
              GGL_OPTIONAL,
              GGL_TYPE_MAP,
              &json_message_obj },
            { GGL_STR("binaryMessage"),
              GGL_OPTIONAL,
              GGL_TYPE_MAP,
              &binary_message_obj },
        )
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Received invalid pubsub subscription response.");
        return GGL_ERR_INVALID;
    }

    if ((json_message_obj == NULL) == (binary_message_obj == NULL)) {
        GGL_LOGE("Received invalid pubsub subscription response.");
        return GGL_ERR_INVALID;
    }

    bool is_json = json_message_obj != NULL;

    GglObject *message_obj;
    GglObject *context_obj;
    ret = ggl_map_validate(
        ggl_obj_into_map(is_json ? *json_message_obj : *binary_message_obj),
        GGL_MAP_SCHEMA(
            { GGL_STR("message"),
              GGL_REQUIRED,
              is_json ? GGL_TYPE_MAP : GGL_TYPE_BUF,
              &message_obj },
            { GGL_STR("context"), GGL_REQUIRED, GGL_TYPE_MAP, &context_obj },
        )
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Received invalid pubsub subscription response.");
        return GGL_ERR_INVALID;
    }

    GglObject *topic_obj;
    ret = ggl_map_validate(
        ggl_obj_into_map(*context_obj),
        GGL_MAP_SCHEMA(
            { GGL_STR("topic"), GGL_REQUIRED, GGL_TYPE_BUF, &topic_obj },
        )
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Received invalid pubsub subscription response.");
        return GGL_ERR_INVALID;
    }

    GglBuffer topic = ggl_obj_into_buf(*topic_obj);

    GglObject payload = *message_obj;

    if (!is_json) {
        GglBuffer payload_buf = ggl_obj_into_buf(*message_obj);
        if (!ggl_base64_decode_in_place(&payload_buf)) {
            GGL_LOGE("Failed to decode pubsub subscription response payload.");
            return GGL_ERR_INVALID;
        }
        payload = ggl_obj_buf(payload_buf);
    }

    callback(topic, payload, handle);
    return GGL_ERR_OK;
}

static GglError error_handler(
    void *ctx, GglBuffer error_code, GglBuffer message
) {
    (void) ctx;

    GGL_LOGE(
        "Received SubscribeToTopic error %.*s: %.*s.",
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

GglError ggipc_subscribe_to_topic(
    GglBuffer topic,
    GgIpcSubscribeToTopicCallback callback,
    GgIpcSubscriptionHandle *handle
) {
    GglMap args = GGL_MAP(ggl_kv(GGL_STR("topic"), ggl_obj_buf(topic)), );

    return ggipc_subscribe(
        GGL_STR("aws.greengrass#SubscribeToTopic"),
        GGL_STR("aws.greengrass#SubscribeToTopicRequest"),
        args,
        NULL,
        &error_handler,
        NULL,
        &subscribe_to_topic_resp_handler,
        callback,
        handle
    );
}
