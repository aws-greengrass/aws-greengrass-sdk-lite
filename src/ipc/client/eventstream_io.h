// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_CLIENT_IO_H
#define GGL_IPC_CLIENT_IO_H

#include <ggl/attr.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/eventstream/decode.h>
#include <ggl/eventstream/types.h>
#include <ggl/object.h>
#include <stddef.h>

GglError ggipc_send_message(
    int conn,
    const EventStreamHeader *headers,
    size_t headers_len,
    GglMap *payload,
    GglBuffer send_buffer
) NONNULL_IF_NONZERO(2, 3);

GglError ggipc_get_message(
    int conn, EventStreamMessage *msg, GglBuffer recv_buffer
) NONNULL(2);

#endif
