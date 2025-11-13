// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_IPC_CLIENT_RAW_H
#define GG_IPC_CLIENT_RAW_H

#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/ipc/client.h>
#include <gg/object.h>

/// Callback invoked on successful IPC response.
typedef GgError GgIpcResultCallback(void *ctx, GgMap result);
/// Callback invoked on error IPC response.
typedef GgError GgIpcErrorCallback(
    void *ctx, GgBuffer error_code, GgBuffer message
);

/// Make a raw IPC call to Greengrass Nucleus.
/// Invokes `result_callback` on success or `error_callback` on error.
/// Returns GG_ERR_NOCONN if not connected, GG_ERR_NOMEM if insufficient
/// resources, or GG_ERR_OK on success.
GgError ggipc_call(
    GgBuffer operation,
    GgBuffer service_model_type,
    GgMap params,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_ctx
);

/// Callback invoked for each subscription event.
typedef GgError GgIpcSubscribeCallback(
    void *ctx,
    void *aux_ctx,
    GgIpcSubscriptionHandle handle,
    GgBuffer service_model_type,
    GgMap data
);

/// Make a raw IPC subscription call to Greengrass Nucleus.
/// Invokes `result_callback` on success or `error_callback` on error.
/// Invokes `sub_callback` for each subscription event.
/// If `sub_handle` is not NULL, sets it to the subscription handle on success.
/// Returns GG_ERR_NOCONN if not connected, GG_ERR_NOMEM if insufficient
/// resources, GG_ERR_INVALID if called from subscription callback,
/// or GG_ERR_OK on success.
GgError ggipc_subscribe(
    GgBuffer operation,
    GgBuffer service_model_type,
    GgMap params,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_ctx,
    GgIpcSubscribeCallback *sub_callback,
    void *sub_callback_ctx,
    void *sub_callback_aux_ctx,
    GgIpcSubscriptionHandle *sub_handle
);

#endif
