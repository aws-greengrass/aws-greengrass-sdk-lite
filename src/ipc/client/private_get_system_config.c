// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/arena.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/client.h>
#include <ggl/ipc/error.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>

GglError ggipc_private_get_system_config(
    int conn, GglBuffer key, GglBuffer *value
) {
    GglArena alloc = ggl_arena_init(*value);
    GglObject resp = { 0 };
    GglIpcError remote_error = GGL_IPC_ERROR_DEFAULT;
    GglError ret = ggipc_call(
        conn,
        GGL_STR("aws.greengrass.private#GetSystemConfig"),
        GGL_STR("aws.greengrass.private#GetSystemConfigRequest"),
        GGL_MAP(ggl_kv(GGL_STR("key"), ggl_obj_buf(key))),
        &alloc,
        &resp,
        &remote_error
    );
    if (ret == GGL_ERR_REMOTE) {
        if (remote_error.error_code != GGL_IPC_ERR_INVALID_ARGUMENTS) {
            GGL_LOGE(
                "Invalid arguments: %.*s",
                (int) remote_error.message.len,
                remote_error.message.data
            );
            return GGL_ERR_INVALID;
        }

        GGL_LOGE("Server error.");
        return GGL_ERR_FAILURE;
    }

    if (ret != GGL_ERR_OK) {
        return ret;
    }

    if (ggl_obj_type(resp) != GGL_TYPE_BUF) {
        GGL_LOGE("Config value is not a string.");
        return GGL_ERR_FAILURE;
    }

    *value = ggl_obj_into_buf(resp);

    GGL_LOGT(
        "Read %.*s: %.*s.",
        (int) key.len,
        key.data,
        (int) value->len,
        value->data
    );

    return GGL_ERR_OK;
}
