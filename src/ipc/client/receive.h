// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_CLIENT_RECEIVE_H
#define GGL_IPC_CLIENT_RECEIVE_H

#include <ggl/attr.h>
#include <ggl/error.h>
#include <ggl/eventstream/decode.h>
#include <ggl/eventstream/rpc.h>
#include <stdint.h>

typedef GglError (*GglIpcStreamHandler)(
    void *ctx, EventStreamCommonHeaders common_headers, EventStreamMessage msg
);

GglError ggipc_register_ipc_socket(int conn);

GglError ggipc_set_stream_handler(
    GglIpcStreamHandler handler, void *ctx, int32_t *stream
) NONNULL(3);

void ggipc_set_stream_handler_at(
    int32_t stream, GglIpcStreamHandler handler, void *ctx
);

#endif
