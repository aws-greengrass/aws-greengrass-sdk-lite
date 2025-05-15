// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_CLIENT_H
#define GGL_IPC_CLIENT_H

#include <ggl/arena.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/object.h>
#include <time.h> // IWYU pragma: keep
#include <stdint.h>

struct timespec;

/// Connect to the Greengrass Nucleus from a component.
/// Not thread-safe due to use of getenv.
GglError ggipc_connect(void);

/// Connect to a GG-IPC socket with a given SVCUID token.
GglError ggipc_connect_with_token(GglBuffer socket_path, GglBuffer auth_token);

GglError ggipc_get_config_str(
    GglBufList key_path, GglBuffer *component_name, GglBuffer *value
);

GglError ggipc_get_config_obj(
    GglBufList key_path,
    GglBuffer *component_name,
    GglArena *alloc,
    GglObject *value
);

GglError ggipc_update_config(
    GglBufList key_path,
    const struct timespec *timestamp,
    GglObject value_to_merge
);

/// Uses an allocator to base64-encode a binary message.
/// base64 encoding will allocate 4 bytes for every 3 payload bytes.
/// Additionally, up to 128 bytes may be allocated for an error message.
GglError ggipc_publish_to_topic_binary(
    GglBuffer topic, GglBuffer payload, GglArena alloc
);

GglError ggipc_publish_to_topic_obj(GglBuffer topic, GglObject payload);

/// Uses an allocator to base64-encode a binary message.
/// base64 encoding will allocate 4 bytes for every 3 payload bytes.
/// Additionally, up to 128 bytes may be allocated for an error message.
GglError ggipc_publish_to_iot_core(
    GglBuffer topic_name, GglBuffer payload, uint8_t qos, GglArena *alloc
);

#endif
