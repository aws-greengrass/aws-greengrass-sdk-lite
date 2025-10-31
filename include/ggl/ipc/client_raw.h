// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_CLIENT_RAW_H
#define GGL_IPC_CLIENT_RAW_H

#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/client.h>
#include <ggl/object.h>

/// Callback invoked on successful IPC response.
typedef GglError GgIpcResultCallback(void *ctx, GglMap result);
/// Callback invoked on error IPC response.
typedef GglError GgIpcErrorCallback(
    void *ctx, GglBuffer error_code, GglBuffer message
);

/// Make a raw IPC call to Greengrass Nucleus.
/// Invokes `result_callback` on success or `error_callback` on error.
/// Returns GGL_ERR_NOCONN if not connected, GGL_ERR_NOMEM if insufficient
/// resources, or GGL_ERR_OK on success.
GglError ggipc_call(
    GglBuffer operation,
    GglBuffer service_model_type,
    GglMap params,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_ctx
);

/// Callback invoked for each subscription event.
typedef GglError GgIpcSubscribeCallback(
    void *ctx,
    void *aux_ctx,
    GgIpcSubscriptionHandle handle,
    GglBuffer service_model_type,
    GglMap data
);

/// Make a raw IPC subscription call to Greengrass Nucleus.
/// Invokes `result_callback` on success or `error_callback` on error.
/// Invokes `sub_callback` for each subscription event.
/// If `sub_handle` is not NULL, sets it to the subscription handle on success.
/// Returns GGL_ERR_NOCONN if not connected, GGL_ERR_NOMEM if insufficient
/// resources, GGL_ERR_INVALID if called from subscription callback,
/// or GGL_ERR_OK on success.
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
