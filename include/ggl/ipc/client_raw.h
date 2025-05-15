// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_CLIENT_RAW_H
#define GGL_IPC_CLIENT_RAW_H

#include <ggl/arena.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/error.h>
#include <ggl/object.h>

#define GGL_IPC_SVCUID_STR_LEN (16)

typedef struct {
    GglError (*fn)(void *ctx, GglBuffer service_model_type, GglObject data);
    void *ctx;
} GgIpcSubscribeCallback;

GglError ggipc_call(
    int conn,
    GglBuffer operation,
    GglBuffer service_model_type,
    GglMap params,
    GglArena *alloc,
    GglObject *result,
    GglIpcError *remote_err
);

GglError ggipc_subscribe(
    int conn,
    GglBuffer operation,
    GglBuffer service_model_type,
    GglMap params,
    const GgIpcSubscribeCallback *on_response,
    GglArena *alloc,
    GglObject *result,
    GglIpcError *remote_err
);

#endif
