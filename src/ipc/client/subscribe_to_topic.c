// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/base64.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/flags.h>
#include <gg/ipc/client.h>
#include <gg/ipc/client_raw.h>
#include <gg/log.h>
#include <gg/map.h>
#include <gg/object.h>
#include <string.h>
#include <stdbool.h>

static GgError subscribe_to_topic_resp_handler(
    void *ctx,
    void *aux_ctx,
    GgIpcSubscriptionHandle handle,
    GgBuffer service_model_type,
    GgMap data
) {
    GgIpcSubscribeToTopicCallback *callback = ctx;

    if (!gg_buffer_eq(
            service_model_type,
            GG_STR("aws.greengrass#SubscriptionResponseMessage")
        )) {
        GG_LOGE("Unexpected service-model-type received.");
        return GG_ERR_INVALID;
    }

    GgObject *json_message_obj;
    GgObject *binary_message_obj;
    GgError ret = gg_map_validate(
        data,
        GG_MAP_SCHEMA(
            { GG_STR("jsonMessage"),
              GG_OPTIONAL,
              GG_TYPE_MAP,
              &json_message_obj },
            { GG_STR("binaryMessage"),
              GG_OPTIONAL,
              GG_TYPE_MAP,
              &binary_message_obj },
        )
    );
    if (ret != GG_ERR_OK) {
        GG_LOGE("Received invalid pubsub subscription response.");
        return GG_ERR_INVALID;
    }

    if ((json_message_obj == NULL) == (binary_message_obj == NULL)) {
        GG_LOGE("Received invalid pubsub subscription response.");
        return GG_ERR_INVALID;
    }

    bool is_json = json_message_obj != NULL;

    GgObject *message_obj;
    GgObject *context_obj;
    ret = gg_map_validate(
        gg_obj_into_map(is_json ? *json_message_obj : *binary_message_obj),
        GG_MAP_SCHEMA(
            { GG_STR("message"),
              GG_REQUIRED,
              is_json ? GG_TYPE_MAP : GG_TYPE_BUF,
              &message_obj },
            { GG_STR("context"), GG_REQUIRED, GG_TYPE_MAP, &context_obj },
        )
    );
    if (ret != GG_ERR_OK) {
        GG_LOGE("Received invalid pubsub subscription response.");
        return GG_ERR_INVALID;
    }

    GgObject *topic_obj;
    ret = gg_map_validate(
        gg_obj_into_map(*context_obj),
        GG_MAP_SCHEMA(
            { GG_STR("topic"), GG_REQUIRED, GG_TYPE_BUF, &topic_obj },
        )
    );
    if (ret != GG_ERR_OK) {
        GG_LOGE("Received invalid pubsub subscription response.");
        return GG_ERR_INVALID;
    }

    GgBuffer topic = gg_obj_into_buf(*topic_obj);

    GgObject payload = *message_obj;

    if (!is_json) {
        GgBuffer payload_buf = gg_obj_into_buf(*message_obj);
        if (!gg_base64_decode_in_place(&payload_buf)) {
            GG_LOGE("Failed to decode pubsub subscription response payload.");
            return GG_ERR_INVALID;
        }
        payload = gg_obj_buf(payload_buf);
    }

    callback(aux_ctx, topic, payload, handle);
    return GG_ERR_OK;
}

static GgError error_handler(void *ctx, GgBuffer error_code, GgBuffer message) {
    (void) ctx;

    GG_LOGE(
        "Received SubscribeToTopic error %.*s: %.*s.",
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

GgError ggipc_subscribe_to_topic(
    GgBuffer topic,
    GgIpcSubscribeToTopicCallback callback,
    void *ctx,
    GgIpcSubscriptionHandle *handle
) {
    GgMap args = GG_MAP(gg_kv(GG_STR("topic"), gg_obj_buf(topic)), );

    return ggipc_subscribe(
        GG_STR("aws.greengrass#SubscribeToTopic"),
        GG_STR("aws.greengrass#SubscribeToTopicRequest"),
        args,
        NULL,
        &error_handler,
        NULL,
        &subscribe_to_topic_resp_handler,
        callback,
        ctx,
        handle
    );
}
