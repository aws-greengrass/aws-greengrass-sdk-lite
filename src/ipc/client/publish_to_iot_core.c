// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/arena.h>
#include <ggl/base64.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/client.h>
#include <ggl/ipc/client_raw.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <inttypes.h>
#include <string.h>

static GglError error_handler(
    void *ctx, GglBuffer error_code, GglBuffer message
) {
    (void) ctx;

    GGL_LOGE(
        "Received PublishToIoTCore error %.*s: %.*s.",
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

// TODO: use GglByteVec for payload to allow in-place base64 encoding.
GglError ggipc_publish_to_iot_core(
    GglBuffer topic_name, GglBuffer payload, uint8_t qos, GglArena alloc
) {
    if (qos > 2) {
        GGL_LOGE("Invalid QoS \"%" PRIu8 "\" provided. QoS must be <= 2", qos);
        return GGL_ERR_INVALID;
    }
    GGL_LOGT("Topic name len: %zu", topic_name.len);
    GglBuffer qos_buffer = GGL_BUF((uint8_t[1]) { qos + (uint8_t) '0' });
    GglBuffer encoded_payload;
    GglError ret = ggl_base64_encode(payload, &alloc, &encoded_payload);
    if (ret != GGL_ERR_OK) {
        return ret;
    }
    GglMap args = GGL_MAP(
        ggl_kv(GGL_STR("topicName"), ggl_obj_buf(topic_name)),
        ggl_kv(GGL_STR("payload"), ggl_obj_buf(encoded_payload)),
        ggl_kv(GGL_STR("qos"), ggl_obj_buf(qos_buffer))
    );

    return ggipc_call(
        GGL_STR("aws.greengrass#PublishToIoTCore"),
        GGL_STR("aws.greengrass#PublishToIoTCoreRequest"),
        args,
        NULL,
        &error_handler,
        NULL
    );
}
