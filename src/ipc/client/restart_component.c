// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/flags.h>
#include <ggl/ipc/client.h>
#include <ggl/ipc/client_raw.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <stddef.h>

static GglError error_handler(
    void *ctx, GglBuffer error_code, GglBuffer message
) {
    (void) ctx;

    GGL_LOGE(
        "Received RestartComponent error %.*s: %.*s.",
        (int) error_code.len,
        error_code.data,
        (int) message.len,
        message.data
    );

    return GGL_ERR_FAILURE;
}

static GglError response_handler(void *ctx, GglMap response) {
    (void) ctx;

    GglObject *restart_status_obj;
    GglError ret = ggl_map_validate(
        response,
        GGL_MAP_SCHEMA({ GGL_STR("restartStatus"),
                         GGL_REQUIRED,
                         GGL_TYPE_BUF,
                         &restart_status_obj })
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("RestartComponent response missing restartStatus.");
        return GGL_ERR_FAILURE;
    }

    GglBuffer restart_status = ggl_obj_into_buf(*restart_status_obj);
    if (ggl_buffer_eq(restart_status, GGL_STR("FAILED"))) {
        GGL_LOGE("Component restart failed.");
        return GGL_ERR_FAILURE;
    }

    return GGL_ERR_OK;
}

GglError ggipc_restart_component(GglBuffer component_name) {
    GglMap args
        = GGL_MAP(ggl_kv(GGL_STR("componentName"), ggl_obj_buf(component_name))
        );

    return ggipc_call(
        GGL_STR("aws.greengrass#RestartComponent"),
        GGL_STR("aws.greengrass#RestartComponentRequest"),
        args,
        &response_handler,
        &error_handler,
        NULL
    );
}
