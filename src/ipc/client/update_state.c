// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/buffer.h>
#include <ggl/error.h>
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
        "Received UpdateState error %.*s: %.*s.",
        (int) error_code.len,
        error_code.data,
        (int) message.len,
        message.data
    );

    return GGL_ERR_FAILURE;
}

GglError ggipc_update_state(GglComponentState state) {
    // Convert enum to string
    GglBuffer state_str;
    switch (state) {
    case GGL_COMPONENT_STATE_RUNNING:
        state_str = GGL_STR("RUNNING");
        break;
    case GGL_COMPONENT_STATE_ERRORED:
        state_str = GGL_STR("ERRORED");
        break;
    default:
        GGL_LOGE("Invalid component state: %d", state);
        return GGL_ERR_INVALID;
    }

    GglMap args = GGL_MAP(ggl_kv(GGL_STR("state"), ggl_obj_buf(state_str)));

    return ggipc_call(
        GGL_STR("aws.greengrass#UpdateState"),
        GGL_STR("aws.greengrass#UpdateStateRequest"),
        args,
        NULL,
        &error_handler,
        NULL
    );
}
