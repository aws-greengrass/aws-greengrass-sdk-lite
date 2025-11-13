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
#include <inttypes.h>
#include <string.h>

static GgError subscribe_to_iot_core_resp_handler(
    void *ctx,
    void *aux_ctx,
    GgIpcSubscriptionHandle handle,
    GgBuffer service_model_type,
    GgMap data
) {
    GgIpcSubscribeToIotCoreCallback *callback = ctx;

    if (!gg_buffer_eq(
            service_model_type, GG_STR("aws.greengrass#IoTCoreMessage")
        )) {
        GG_LOGE("Unexpected service-model-type received.");
        return GG_ERR_INVALID;
    }

    GgObject *message_obj;
    GgError ret = gg_map_validate(
        data,
        GG_MAP_SCHEMA(
            { GG_STR("message"), GG_REQUIRED, GG_TYPE_MAP, &message_obj }
        )
    );
    if (ret != GG_ERR_OK) {
        GG_LOGE("Received invalid IoT Core subscription response.");
        return GG_ERR_INVALID;
    }
    GgMap message = gg_obj_into_map(*message_obj);

    GgObject *topic_obj;
    GgObject *payload_obj;
    ret = gg_map_validate(
        message,
        GG_MAP_SCHEMA(
            { GG_STR("topicName"), GG_REQUIRED, GG_TYPE_BUF, &topic_obj },
            { GG_STR("payload"), GG_REQUIRED, GG_TYPE_BUF, &payload_obj },
        )
    );
    if (ret != GG_ERR_OK) {
        GG_LOGE("Received invalid IoT Core subscription response.");
        return GG_ERR_INVALID;
    }
    GgBuffer topic = gg_obj_into_buf(*topic_obj);
    GgBuffer payload = gg_obj_into_buf(*payload_obj);

    if (!gg_base64_decode_in_place(&payload)) {
        GG_LOGE("Failed to decode IoT Core subscription response payload.");
        return GG_ERR_INVALID;
    }

    callback(aux_ctx, topic, payload, handle);
    return GG_ERR_OK;
}

static GgError error_handler(void *ctx, GgBuffer error_code, GgBuffer message) {
    (void) ctx;

    GG_LOGE(
        "Received SubscribeToIoTCore error %.*s: %.*s.",
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

GgError ggipc_subscribe_to_iot_core(
    GgBuffer topic_filter,
    uint8_t qos,
    GgIpcSubscribeToIotCoreCallback *callback,
    void *ctx,
    GgIpcSubscriptionHandle *handle
) {
    if (qos > 2) {
        GG_LOGE("Invalid QoS \"%" PRIu8 "\" provided. QoS must be <= 2", qos);
        return GG_ERR_INVALID;
    }
    GgBuffer qos_buffer = GG_BUF((uint8_t[1]) { qos + (uint8_t) '0' });
    GgMap args = GG_MAP(
        gg_kv(GG_STR("topicName"), gg_obj_buf(topic_filter)),
        gg_kv(GG_STR("qos"), gg_obj_buf(qos_buffer))
    );

    return ggipc_subscribe(
        GG_STR("aws.greengrass#SubscribeToIoTCore"),
        GG_STR("aws.greengrass#SubscribeToIoTCoreRequest"),
        args,
        NULL,
        &error_handler,
        NULL,
        &subscribe_to_iot_core_resp_handler,
        callback,
        ctx,
        handle
    );
}
