// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_CLIENT_RAW_H
#define GGL_IPC_CLIENT_RAW_H

#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/client.h>
#include <ggl/object.h>

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
    void *ctx,
    void *aux_ctx,
    GgIpcSubscriptionHandle handle,
    GglBuffer service_model_type,
    GglMap data
);

GglError ggipc_subscribe(
    GglBuffer operation,
    GglBuffer service_model_type,
    GglMap params,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_ctx,
    GgIpcSubscribeCallback *sub_callback,
    void *sub_callback_ctx,
    void *sub_callback_aux_ctx,
    GgIpcSubscriptionHandle *sub_handle
);

#endif
