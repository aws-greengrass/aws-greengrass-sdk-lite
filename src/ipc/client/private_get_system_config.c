// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/arena.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/flags.h>
#include <ggl/ipc/client_priv.h>
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
        "Received PrivateGetSystemConfig error %.*s: %.*s.",
        (int) error_code.len,
        error_code.data,
        (int) message.len,
        message.data
    );

    return GGL_ERR_FAILURE;
}

static GglError copy_config_buf(void *ctx, GglObject result) {
    GglBuffer *resp_buf = ctx;

    if (ggl_obj_type(result) != GGL_TYPE_MAP) {
        GGL_LOGE("Config response is not a map.");
        return GGL_ERR_FAILURE;
    }

    GglObject *value;
    GglError ret = ggl_map_validate(
        ggl_obj_into_map(result),
        GGL_MAP_SCHEMA({ GGL_STR("value"), GGL_REQUIRED, GGL_TYPE_NULL, &value }
        )
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed validating server response.");
        return GGL_ERR_INVALID;
    }

    if (ggl_obj_type(*value) != GGL_TYPE_BUF) {
        GGL_LOGE("Config value is not a string.");
        return GGL_ERR_FAILURE;
    }

    if (resp_buf != NULL) {
        GglBuffer val_buf = ggl_obj_into_buf(*value);

        GglArena alloc = ggl_arena_init(*resp_buf);
        ret = ggl_arena_claim_buf(&val_buf, &alloc);
        if (ret != GGL_ERR_OK) {
            GGL_LOGE("Insufficent memory provided for response.");
            return ret;
        }

        *resp_buf = val_buf;
    }

    return GGL_ERR_OK;
}

GglError ggipc_private_get_system_config(GglBuffer key, GglBuffer *value) {
    return ggipc_call(
        GGL_STR("aws.greengrass.private#GetSystemConfig"),
        GGL_STR("aws.greengrass.private#GetSystemConfigRequest"),
        GGL_MAP(ggl_kv(GGL_STR("key"), ggl_obj_buf(key))),
        &copy_config_buf,
        &error_handler,
        value
    );
}
