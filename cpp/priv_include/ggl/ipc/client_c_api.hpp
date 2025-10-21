#ifndef GGL_IPC_CLIENT_C_API
#define GGL_IPC_CLIENT_C_API

#include <ggl/error.hpp>
#include <ggl/types.hpp>
#include <stdint.h>

extern "C" {

GglError ggipc_connect(void) noexcept;

GglError ggipc_connect_with_token(
    GglBuffer socket_path, GglBuffer auth_token
) noexcept;

GglError ggipc_publish_to_topic_json(GglBuffer topic, GglMap payload) noexcept;

GglError ggipc_publish_to_topic_binary(
    GglBuffer topic, GglBuffer payload
) noexcept;

GglError ggipc_publish_to_topic_binary_b64(
    GglBuffer topic, GglBuffer b64_payload
) noexcept;

typedef void GgIpcSubscribeToTopicCallback(
    void *ctx,
    GglBuffer topic,
    GglObject payload,
    GgIpcSubscriptionHandle handle
);

GglError ggipc_subscribe_to_topic(
    GglBuffer topic,
    GgIpcSubscribeToTopicCallback *callback,
    void *ctx,
    GgIpcSubscriptionHandle *handle
) noexcept;

GglError ggipc_publish_to_iot_core(
    GglBuffer topic_name, GglBuffer payload, uint8_t qos
) noexcept;

GglError ggipc_publish_to_iot_core_b64(
    GglBuffer topic_name, GglBuffer b64_payload, uint8_t qos
) noexcept;

typedef void GgIpcSubscribeToIotCoreCallback(
    void *ctx,
    GglBuffer topic,
    GglBuffer payload,
    GgIpcSubscriptionHandle handle
) noexcept;

GglError ggipc_subscribe_to_iot_core(
    GglBuffer topic_filter,
    uint8_t qos,
    GgIpcSubscribeToIotCoreCallback *callback,
    void *ctx,
    GgIpcSubscriptionHandle *handle
) noexcept;

GglError ggipc_get_config(
    GglBufList key_path,
    const GglBuffer *component_name,
    GglArena *alloc,
    GglObject *value
) noexcept;

GglError ggipc_get_config_str(
    GglBufList key_path, const GglBuffer *component_name, GglBuffer *value
) noexcept;

GglError ggipc_update_config(
    GglBufList key_path,
    const struct timespec *timestamp,
    GglObject value_to_merge
) noexcept;

GglError ggipc_update_state(GglComponentState state) noexcept;

GglError ggipc_restart_component(GglBuffer component_name);

typedef void GgIpcSubscribeToConfigurationUpdateCallback(
    void *ctx,
    GglBuffer component_name,
    GglList key_path,
    GgIpcSubscriptionHandle handle
);

GglError ggipc_subscribe_to_configuration_update(
    const GglBuffer *component_name,
    GglBufList key_path,
    GgIpcSubscribeToConfigurationUpdateCallback *callback,
    void *ctx,
    GgIpcSubscriptionHandle *handle
);
}

#endif
