// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_CLIENT_RAW_H
#define GGL_IPC_CLIENT_RAW_H

#include <ggl/arena.h>
#include <ggl/attr.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/error.h>
#include <ggl/object.h>

#define GGL_IPC_SVCUID_STR_LEN (16)

/// Maximum number of eventstream streams. Limits subscriptions.
/// Max subscriptions is this minus 2.
/// Can be configured with `-D GGL_IPC_MAX_MSG_LEN=<N>`.
#ifndef GGL_IPC_MAX_STREAMS
#define GGL_IPC_MAX_STREAMS 16
#endif

GglError ggipc_call(
    GglBuffer operation,
    GglBuffer service_model_type,
    GglMap params,
    GglArena *alloc,
    GglObject *result,
    GglIpcError *remote_err
);

typedef GglError (*GgIpcSubscribeCallback)(
    void *ctx, GglBuffer service_model_type, GglMap data
);

GglError ggipc_subscribe(
    GglBuffer operation,
    GglBuffer service_model_type,
    GglMap params,
    GgIpcSubscribeCallback on_response,
    void *ctx,
    GglArena *alloc,
    GglObject *result,
    GglIpcError *remote_err
) NONNULL(4);

#endif
