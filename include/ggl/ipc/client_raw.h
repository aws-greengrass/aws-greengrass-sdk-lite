// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_CLIENT_RAW_H
#define GGL_IPC_CLIENT_RAW_H

#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/object.h>

/// Maximum number of eventstream streams. Limits subscriptions.
/// Max subscriptions is this minus 2.
/// Can be configured with `-D GGL_IPC_MAX_MSG_LEN=<N>`.
#ifndef GGL_IPC_MAX_STREAMS
#define GGL_IPC_MAX_STREAMS 16
#endif

typedef GglError GgIpcResultCallback(void *ctx, GglMap result);
typedef GglError GgIpcErrorCallback(
    void *ctx, GglBuffer error_code, GglBuffer message
);

GglError ggipc_call(
    GglBuffer operation,
    GglBuffer service_model_type,
    GglMap params,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_ctx
);

typedef GglError GgIpcSubscribeCallback(
    void *ctx, GglBuffer service_model_type, GglMap data
);

GglError ggipc_subscribe(
    GglBuffer operation,
    GglBuffer service_model_type,
    GglMap params,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_callback_ctx,
    GgIpcSubscribeCallback *sub_callback,
    void *sub_callback_ctx
);

#endif
