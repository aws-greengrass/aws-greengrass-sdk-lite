// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_CLIENT_PRIV_H
#define GGL_IPC_CLIENT_PRIV_H

#include <ggl/attr.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/object.h>

/// Connect to GG-IPC server with the given payload.
/// If svcuid is non-null, it will be filled with the component's identity
/// token. Buffer must be able to hold at least GGL_IPC_SVCUID_STR_LEN.
GglError ggipc_connect_with_payload(
    GglBuffer socket_path, GglMap payload, int *fd, GglBuffer *svcuid
) NONNULL(3);

GglError ggipc_private_get_system_config(
    int conn, GglBuffer key, GglBuffer *value
);

#endif
