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
#include <inttypes.h>
#include <string.h>

static GglError subscribe_to_iot_core_resp_handler(
    void *ctx,
    GgIpcSubscriptionHandle handle,
    GglBuffer service_model_type,
    GglMap data
) {
    GgIpcSubscribeToIotCoreCallback *callback = ctx;

    if (!ggl_buffer_eq(
            service_model_type, GGL_STR("aws.greengrass#IoTCoreMessage")
        )) {
        GGL_LOGE("Unexpected service-model-type received.");
        return GGL_ERR_INVALID;
    }

    GglObject *message_obj;
    GglError ret = ggl_map_validate(
        data,
        GGL_MAP_SCHEMA(
            { GGL_STR("message"), GGL_REQUIRED, GGL_TYPE_MAP, &message_obj }
        )
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Received invalid IoT Core subscription response.");
        return GGL_ERR_INVALID;
    }
    GglMap message = ggl_obj_into_map(*message_obj);

    GglObject *topic_obj;
    GglObject *payload_obj;
    ret = ggl_map_validate(
        message,
        GGL_MAP_SCHEMA(
            { GGL_STR("topicName"), GGL_REQUIRED, GGL_TYPE_BUF, &topic_obj },
            { GGL_STR("payload"), GGL_REQUIRED, GGL_TYPE_BUF, &payload_obj },
        )
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Received invalid IoT Core subscription response.");
        return GGL_ERR_INVALID;
    }
    GglBuffer topic = ggl_obj_into_buf(*topic_obj);
    GglBuffer payload = ggl_obj_into_buf(*payload_obj);

    if (!ggl_base64_decode_in_place(&payload)) {
        GGL_LOGE("Failed to decode IoT Core subscription response payload.");
        return GGL_ERR_INVALID;
    }

    callback(topic, payload, handle);
    return GGL_ERR_OK;
}

static GglError error_handler(
    void *ctx, GglBuffer error_code, GglBuffer message
) {
    (void) ctx;

    GGL_LOGE(
        "Received SubscribeToIoTCore error %.*s: %.*s.",
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

GglError ggipc_subscribe_to_iot_core(
    GglBuffer topic_filter,
    uint8_t qos,
    GgIpcSubscribeToIotCoreCallback *callback,
    GgIpcSubscriptionHandle *handle
) {
    if (qos > 2) {
        GGL_LOGE("Invalid QoS \"%" PRIu8 "\" provided. QoS must be <= 2", qos);
        return GGL_ERR_INVALID;
    }
    GglBuffer qos_buffer = GGL_BUF((uint8_t[1]) { qos + (uint8_t) '0' });
    GglMap args = GGL_MAP(
        ggl_kv(GGL_STR("topicName"), ggl_obj_buf(topic_filter)),
        ggl_kv(GGL_STR("qos"), ggl_obj_buf(qos_buffer))
    );

    return ggipc_subscribe(
        GGL_STR("aws.greengrass#SubscribeToIoTCore"),
        GGL_STR("aws.greengrass#SubscribeToIoTCoreRequest"),
        args,
        NULL,
        &error_handler,
        NULL,
        &subscribe_to_iot_core_resp_handler,
        callback,
        handle
    );
}
