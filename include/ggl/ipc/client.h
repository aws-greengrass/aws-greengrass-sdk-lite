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

GglError ggipc_get_config_str(
    int conn, GglBufList key_path, GglBuffer *component_name, GglBuffer *value
);

GglError ggipc_get_config_obj(
    int conn,
    GglBufList key_path,
    GglBuffer *component_name,
    GglArena *alloc,
    GglObject *value
);

GglError ggipc_update_config(
    int conn,
    GglBufList key_path,
    const struct timespec *timestamp,
    GglObject value_to_merge
);

/// Uses an allocator to base64-encode a binary message.
/// base64 encoding will allocate 4 bytes for every 3 payload bytes.
/// Additionally, up to 128 bytes may be allocated for an error message.
GglError ggipc_publish_to_topic_binary(
    int conn, GglBuffer topic, GglBuffer payload, GglArena alloc
);

GglError ggipc_publish_to_topic_obj(
    int conn, GglBuffer topic, GglObject payload
);

/// Uses an allocator to base64-encode a binary message.
/// base64 encoding will allocate 4 bytes for every 3 payload bytes.
/// Additionally, up to 128 bytes may be allocated for an error message.
GglError ggipc_publish_to_iot_core(
    int conn,
    GglBuffer topic_name,
    GglBuffer payload,
    uint8_t qos,
    GglArena *alloc
);

#endif
