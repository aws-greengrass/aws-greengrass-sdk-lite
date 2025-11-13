// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/ipc/client.h>
#include <gg/ipc/client_raw.h>
#include <gg/log.h>
#include <gg/map.h>
#include <gg/object.h>
#include <stddef.h>

static GgError error_handler(void *ctx, GgBuffer error_code, GgBuffer message) {
    (void) ctx;

    GG_LOGE(
        "Received UpdateState error %.*s: %.*s.",
        (int) error_code.len,
        error_code.data,
        (int) message.len,
        message.data
    );

    return GG_ERR_FAILURE;
}

GgError ggipc_update_state(GgComponentState state) {
    // Convert enum to string
    GgBuffer state_str;
    switch (state) {
    case GG_COMPONENT_STATE_RUNNING:
        state_str = GG_STR("RUNNING");
        break;
    case GG_COMPONENT_STATE_ERRORED:
        state_str = GG_STR("ERRORED");
        break;
    default:
        GG_LOGE("Invalid component state: %d", state);
        return GG_ERR_INVALID;
    }

    GgMap args = GG_MAP(gg_kv(GG_STR("state"), gg_obj_buf(state_str)));

    return ggipc_call(
        GG_STR("aws.greengrass#UpdateState"),
        GG_STR("aws.greengrass#UpdateStateRequest"),
        args,
        NULL,
        &error_handler,
        NULL
    );
}
