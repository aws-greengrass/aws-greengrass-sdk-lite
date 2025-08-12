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
#include <stdint.h>

struct timespec;

/// Maximum number of eventstream streams. Limits active calls/subscriptions.
/// Can be configured with `-D GGL_IPC_MAX_STREAMS=<N>`.
#ifndef GGL_IPC_MAX_STREAMS
#define GGL_IPC_MAX_STREAMS 16
#endif

/// Maximum time IPC functions will wait for server response
#ifndef GGL_IPC_RESPONSE_TIMEOUT
#define GGL_IPC_RESPONSE_TIMEOUT 10
#endif

// Connection APIs

/// Connect to the Greengrass Nucleus from a component.
/// Not thread-safe due to use of getenv.
GGL_EXPORT
GglError ggipc_connect(void);

/// Connect to a GG-IPC socket with a given SVCUID token.
GGL_EXPORT
GglError ggipc_connect_with_token(GglBuffer socket_path, GglBuffer auth_token);

// IPC calls

/// Publish a message to a local topic in JSON format
GGL_EXPORT
GglError ggipc_publish_to_topic_json(GglBuffer topic, GglMap payload);

/// Publish a message to a local topic in binary format
/// Usage may incur memory overhead over using `ggipc_publish_to_topic_b64`
GGL_EXPORT
GglError ggipc_publish_to_topic_binary(GglBuffer topic, GglBuffer payload);

/// Publish a message to a local topic in binary format
/// Payload must be already base64 encoded.
GGL_EXPORT
GglError ggipc_publish_to_topic_binary_b64(
    GglBuffer topic, GglBuffer b64_payload
);

typedef struct {
    void (*json_handler)(GglBuffer topic, GglMap payload);
    void (*binary_handler)(GglBuffer topic, GglBuffer payload);
} GgIpcSubscribeToTopicCallbacks;

/// Subscribe to messages on a local topic
/// `handlers` must have static lifetime.
/// `json_handler` or `binary_handler` may be NULL if that payload type is not
/// expected.
GGL_EXPORT
GglError ggipc_subscribe_to_topic(
    GglBuffer topic, const GgIpcSubscribeToTopicCallbacks callbacks[static 1]
) ACCESS(read_only, 2);

/// Publish an MQTT message to AWS IoT Core on a topic
/// Usage may incur memory overhead over using `ggipc_publish_to_iot_core_b64`
GGL_EXPORT
GglError ggipc_publish_to_iot_core(
    GglBuffer topic_name, GglBuffer payload, uint8_t qos
);

/// Publish an MQTT message to AWS IoT Core on a topic
/// Payload must be already base64 encoded.
GGL_EXPORT
GglError ggipc_publish_to_iot_core_b64(
    GglBuffer topic_name, GglBuffer b64_payload, uint8_t qos
);

typedef void GgIpcSubscribeToIotCoreCallback(
    GglBuffer topic, GglBuffer payload
);

/// Subscribe to MQTT messages from AWS IoT Core on a topic or topic filter
GGL_EXPORT
GglError ggipc_subscribe_to_iot_core(
    GglBuffer topic_filter,
    uint8_t qos,
    GgIpcSubscribeToIotCoreCallback *callback
) NONNULL(3);

/// Get a configuration value for a component on the core device
GGL_EXPORT
GglError ggipc_get_config(
    GglBufList key_path,
    const GglBuffer *component_name,
    GglArena *alloc,
    GglObject *value
) ACCESS(read_only, 2) ACCESS(read_write, 3) ACCESS(write_only, 4);

/// Get a string-typed configuration value for a component on the core device
/// `value` must point to a buffer large enough to hold the result, and will be
/// updated to the result string.
/// Alternative API to ggipc_get_config for string type values.
GGL_EXPORT
GglError ggipc_get_config_str(
    GglBufList key_path, const GglBuffer *component_name, GglBuffer *value
) ACCESS(read_only, 2) ACCESS(read_write, 3);

/// Update a configuration value for this component on the core device
GGL_EXPORT
GglError ggipc_update_config(
    GglBufList key_path,
    const struct timespec *timestamp,
    GglObject value_to_merge
) ACCESS(read_only, 2);

/// Component state values for UpdateState
typedef enum ENUM_EXTENSIBILITY(closed) {
    GGL_COMPONENT_STATE_RUNNING,
    GGL_COMPONENT_STATE_ERRORED
} GglComponentState;

/// Update the state of this component
GGL_EXPORT
GglError ggipc_update_state(GglComponentState state);

#endif
