// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_CLIENT_H
#define GGL_IPC_CLIENT_H

#include <ggl/arena.h>
#include <ggl/attr.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/object.h>
#include <time.h> // IWYU pragma: keep
#include <stdint.h>

struct timespec;

// Connection APIs

/// Connect to the Greengrass Nucleus from a component.
/// Not thread-safe due to use of getenv.
GglError ggipc_connect(void);

/// Connect to a GG-IPC socket with a given SVCUID token.
GglError ggipc_connect_with_token(GglBuffer socket_path, GglBuffer auth_token);

// IPC calls

/// Publish a message to a local topic in JSON format
GglError ggipc_publish_to_topic_json(GglBuffer topic, GglMap payload);

/// Publish a message to a local topic in binary format
/// Uses an allocator to base64-encode a binary message.
/// base64 encoding will allocate 4 bytes for every 3 payload bytes.
/// Additionally, up to 128 bytes may be allocated for an error message.
GglError ggipc_publish_to_topic_binary(
    GglBuffer topic, GglBuffer payload, GglArena alloc
);

/// Publish an MQTT message to AWS IoT Core on a topic
/// Uses an allocator to base64-encode a binary message.
/// base64 encoding will allocate 4 bytes for every 3 payload bytes.
/// Additionally, up to 128 bytes may be allocated for an error message.
GglError ggipc_publish_to_iot_core(
    GglBuffer topic_name, GglBuffer payload, uint8_t qos, GglArena alloc
);

typedef void (*GgIpcSubscribeToIotCoreCallback)(
    GglBuffer topic, GglBuffer payload
);

/// Subscribe to MQTT messages from AWS IoT Core on a topic or topic filter
GglError ggipc_subscribe_to_iot_core(
    GglBuffer topic_filter, uint8_t qos, GgIpcSubscribeToIotCoreCallback handler
) NONNULL(3);

/// Get a configuration value for a component on the core device
GglError ggipc_get_config(
    GglBufList key_path,
    const GglBuffer *component_name,
    GglArena *alloc,
    GglObject *value
);

/// Get a string-typed configuration value for a component on the core device
/// Alternative API to ggipc_get_config for string type values.
GglError ggipc_get_config_str(
    GglBufList key_path, const GglBuffer *component_name, GglBuffer *value
);

/// Update a configuration value for this component on the core device
GglError ggipc_update_config(
    GglBufList key_path,
    const struct timespec *timestamp,
    GglObject value_to_merge
);

#endif
