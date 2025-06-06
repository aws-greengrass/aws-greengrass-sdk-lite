// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_CLIENT_PRIV_H
#define GGL_IPC_CLIENT_PRIV_H

#include <ggl/buffer.h>
#include <ggl/error.h>

/// Maximum size of eventstream packet.
/// Can be configured with `-D GGL_IPC_MAX_MSG_LEN=<N>`.
#ifndef GGL_IPC_MAX_MSG_LEN
#define GGL_IPC_MAX_MSG_LEN 10000
#endif

/// Maximum size IPC functions will wait for server response
#ifndef GGL_IPC_RESPONSE_TIMEOUT
#define GGL_IPC_RESPONSE_TIMEOUT 10
#endif

/// Connect to GG-IPC server using component name.
/// If svcuid is non-null, it will be filled with the component's identity
/// token. Buffer must be able to hold at least GGL_IPC_SVCUID_STR_LEN.
GglError ggipc_connect_with_name(
    GglBuffer socket_path, GglBuffer component_name, GglBuffer *svcuid
);

GglError ggipc_private_get_system_config(GglBuffer key, GglBuffer *value);

#endif
