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

/// Maximum time IPC functions will wait for server response.
#ifndef GGL_IPC_RESPONSE_TIMEOUT
#define GGL_IPC_RESPONSE_TIMEOUT 10
#endif

// Connection APIs

/// Connect to the Greengrass Nucleus from a component.
/// Uses SVCUID and AWS_GG_NUCLEUS_DOMAIN_SOCKET_FILEPATH_FOR_COMPONENT
/// environment variables.
/// Not thread-safe due to use of getenv.
GglError ggipc_connect(void);

/// Connect to a GG-IPC socket with a given SVCUID token.
/// Thread-safe alternative to ggipc_connect that does not call getenv.
GglError ggipc_connect_with_token(GglBuffer socket_path, GglBuffer auth_token);

// Subscription management

/// Handle for referring to a subscripion created by an IPC call.
typedef struct {
    uint32_t val;
} GgIpcSubscriptionHandle;

/// Close a subscription returned by an IPC call.
void ggipc_close_subscription(GgIpcSubscriptionHandle handle);

// IPC calls

/// Publish a JSON message to a local pub/sub topic.
/// Sends messages to other Greengrass components subscribed to the topic.
/// Requires aws.greengrass#PublishToTopic authorization.
/// See:
/// <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-publish-subscribe.html#ipc-operation-publishtotopic>
GglError ggipc_publish_to_topic_json(GglBuffer topic, GglMap payload);

/// Publish a binary message to a local pub/sub topic.
/// Sends messages to other Greengrass components subscribed to the topic.
/// Requires aws.greengrass#PublishToTopic authorization.
/// Usage may incur memory overhead over using `ggipc_publish_to_topic_b64`
/// See:
/// <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-publish-subscribe.html#ipc-operation-publishtotopic>
GglError ggipc_publish_to_topic_binary(GglBuffer topic, GglBuffer payload);

/// Publish a binary message to a local pub/sub topic.
/// Payload must be already base64 encoded.
/// Requires aws.greengrass#PublishToTopic authorization.
/// See:
/// <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-publish-subscribe.html#ipc-operation-publishtotopic>
GglError ggipc_publish_to_topic_binary_b64(
    GglBuffer topic, GglBuffer b64_payload
);

typedef void GgIpcSubscribeToTopicCallback(
    void *ctx,
    GglBuffer topic,
    GglObject payload,
    GgIpcSubscriptionHandle handle
);

/// Subscribe to messages on a local pub/sub topic.
/// Receives messages from other Greengrass components publishing to the topic.
/// Payload will be a map for json messages and a buffer for binary messages.
/// Requires aws.greengrass#SubscribeToTopic authorization.
/// See:
/// <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-publish-subscribe.html#ipc-operation-subscribetotopic>
NONNULL(2)
GglError ggipc_subscribe_to_topic(
    GglBuffer topic,
    GgIpcSubscribeToTopicCallback *callback,
    void *ctx,
    GgIpcSubscriptionHandle *handle
);

/// Publish an MQTT message to AWS IoT Core.
/// Sends messages to AWS IoT Core MQTT broker with specified QoS.
/// Requires aws.greengrass#PublishToIoTCore authorization.
/// Usage may incur memory overhead over using `ggipc_publish_to_iot_core_b64`
/// See:
/// <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-iot-core-mqtt.html#ipc-operation-publishtoiotcore>
GglError ggipc_publish_to_iot_core(
    GglBuffer topic_name, GglBuffer payload, uint8_t qos
);

/// Publish an MQTT message to AWS IoT Core.
/// Payload must be already base64 encoded.
/// Requires aws.greengrass#PublishToIoTCore authorization.
/// See:
/// <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-iot-core-mqtt.html#ipc-operation-publishtoiotcore>
GglError ggipc_publish_to_iot_core_b64(
    GglBuffer topic_name, GglBuffer b64_payload, uint8_t qos
);

typedef void GgIpcSubscribeToIotCoreCallback(
    void *ctx,
    GglBuffer topic,
    GglBuffer payload,
    GgIpcSubscriptionHandle handle
);

/// Subscribe to MQTT messages from AWS IoT Core.
/// Receives messages from AWS IoT Core MQTT broker on matching topics.
/// Requires aws.greengrass#SubscribeToIoTCore authorization.
/// See:
/// <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-iot-core-mqtt.html#ipc-operation-subscribetoiotcore>
NONNULL(3)
GglError ggipc_subscribe_to_iot_core(
    GglBuffer topic_filter,
    uint8_t qos,
    GgIpcSubscribeToIotCoreCallback *callback,
    void *ctx,
    GgIpcSubscriptionHandle *handle
);

/// Get component configuration value.
/// Retrieves configuration for the specified key path.
/// Pass empty list for complete config.
/// Requires aws.greengrass#GetConfiguration authorization.
/// See:
/// <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-component-configuration.html#ipc-operation-getconfiguration>
ACCESS(read_only, 2) ACCESS(read_write, 3) ACCESS(write_only, 4)
GglError ggipc_get_config(
    GglBufList key_path,
    const GglBuffer *component_name,
    GglArena *alloc,
    GglObject *value
);

/// Get component configuration value as a string.
/// `value` must point to a buffer large enough to hold the result, and will be
/// updated to the result string.
/// Alternative API to ggipc_get_config for string type values.
/// Requires aws.greengrass#GetConfiguration authorization.
/// See:
/// <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-component-configuration.html#ipc-operation-getconfiguration>
ACCESS(read_only, 2) ACCESS(read_write, 3)
GglError ggipc_get_config_str(
    GglBufList key_path, const GglBuffer *component_name, GglBuffer *value
);

/// Update component configuration.
/// Merges the provided value into the component's configuration at the key
/// path. Requires aws.greengrass#UpdateConfiguration authorization. See:
/// <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-component-configuration.html#ipc-operation-updateconfiguration>
ACCESS(read_only, 2)
GglError ggipc_update_config(
    GglBufList key_path,
    const struct timespec *timestamp,
    GglObject value_to_merge
);

/// Component state values for UpdateState
typedef enum ENUM_EXTENSIBILITY(closed) {
    GGL_COMPONENT_STATE_RUNNING,
    GGL_COMPONENT_STATE_ERRORED
} GglComponentState;

/// Update the state of this component.
/// Reports component state to the Greengrass nucleus.
/// See:
/// <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-component-lifecycle.html
GglError ggipc_update_state(GglComponentState state);

/// Restart a Greengrass component.
/// Requests the nucleus to restart the specified component.
/// See:
/// <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-component-lifecycle.html
GglError ggipc_restart_component(GglBuffer component_name);

typedef void GgIpcSubscribeToConfigurationUpdateCallback(
    void *ctx,
    GglBuffer component_name,
    GglList key_path,
    GgIpcSubscriptionHandle handle
);

/// Subscribe to component configuration updates.
/// Receives notifications when configuration changes for the specified key
/// path. Pass NULL for component_name to refer to current component. Requires
/// aws.greengrass#SubscribeToConfigurationUpdate authorization. See:
/// <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-component-configuration.html#ipc-operation-subscribetoconfigurationupdate>
ACCESS(read_only, 1) NONNULL(3)
GglError ggipc_subscribe_to_configuration_update(
    const GglBuffer *component_name,
    GglBufList key_path,
    GgIpcSubscribeToConfigurationUpdateCallback *callback,
    void *ctx,
    GgIpcSubscriptionHandle *handle
);

#endif
