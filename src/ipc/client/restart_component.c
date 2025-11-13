// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/flags.h>
#include <gg/ipc/client.h>
#include <gg/ipc/client_raw.h>
#include <gg/log.h>
#include <gg/map.h>
#include <gg/object.h>
#include <stddef.h>

static GgError error_handler(void *ctx, GgBuffer error_code, GgBuffer message) {
    (void) ctx;

    GG_LOGE(
        "Received RestartComponent error %.*s: %.*s.",
        (int) error_code.len,
        error_code.data,
        (int) message.len,
        message.data
    );

    return GG_ERR_FAILURE;
}

static GgError response_handler(void *ctx, GgMap response) {
    (void) ctx;

    GgObject *restart_status_obj;
    GgError ret = gg_map_validate(
        response,
        GG_MAP_SCHEMA(
            { GG_STR("restartStatus"),
              GG_REQUIRED,
              GG_TYPE_BUF,
              &restart_status_obj }
        )
    );
    if (ret != GG_ERR_OK) {
        GG_LOGE("RestartComponent response missing restartStatus.");
        return GG_ERR_FAILURE;
    }

    GgBuffer restart_status = gg_obj_into_buf(*restart_status_obj);
    if (gg_buffer_eq(restart_status, GG_STR("FAILED"))) {
        GG_LOGE("Component restart failed.");
        return GG_ERR_FAILURE;
    }

    return GG_ERR_OK;
}

GgError ggipc_restart_component(GgBuffer component_name) {
    GgMap args
        = GG_MAP(gg_kv(GG_STR("componentName"), gg_obj_buf(component_name)));

    return ggipc_call(
        GG_STR("aws.greengrass#RestartComponent"),
        GG_STR("aws.greengrass#RestartComponentRequest"),
        args,
        &response_handler,
        &error_handler,
        NULL
    );
}
