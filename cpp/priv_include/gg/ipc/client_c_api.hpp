#ifndef GG_IPC_CLIENT_C_API
#define GG_IPC_CLIENT_C_API

#include <gg/error.hpp>
#include <gg/types.hpp>
#include <stdint.h>

extern "C" {

GgError ggipc_connect(void) noexcept;

GgError ggipc_connect_with_token(
    GgBuffer socket_path, GgBuffer auth_token
) noexcept;

GgError ggipc_publish_to_topic_json(GgBuffer topic, GgMap payload) noexcept;

GgError ggipc_publish_to_topic_binary(
    GgBuffer topic, GgBuffer payload
) noexcept;

GgError ggipc_publish_to_topic_binary_b64(
    GgBuffer topic, GgBuffer b64_payload
) noexcept;

typedef void GgIpcSubscribeToTopicCallback(
    void *ctx, GgBuffer topic, GgObject payload, GgIpcSubscriptionHandle handle
);

GgError ggipc_subscribe_to_topic(
    GgBuffer topic,
    GgIpcSubscribeToTopicCallback *callback,
    void *ctx,
    GgIpcSubscriptionHandle *handle
) noexcept;

GgError ggipc_publish_to_iot_core(
    GgBuffer topic_name, GgBuffer payload, uint8_t qos
) noexcept;

GgError ggipc_publish_to_iot_core_b64(
    GgBuffer topic_name, GgBuffer b64_payload, uint8_t qos
) noexcept;

typedef void GgIpcSubscribeToIotCoreCallback(
    void *ctx, GgBuffer topic, GgBuffer payload, GgIpcSubscriptionHandle handle
) noexcept;

GgError ggipc_subscribe_to_iot_core(
    GgBuffer topic_filter,
    uint8_t qos,
    GgIpcSubscribeToIotCoreCallback *callback,
    void *ctx,
    GgIpcSubscriptionHandle *handle
) noexcept;

GgError ggipc_get_config(
    GgBufList key_path,
    const GgBuffer *component_name,
    GgArena *alloc,
    GgObject *value
) noexcept;

GgError ggipc_get_config_str(
    GgBufList key_path, const GgBuffer *component_name, GgBuffer *value
) noexcept;

GgError ggipc_update_config(
    GgBufList key_path,
    const struct timespec *timestamp,
    GgObject value_to_merge
) noexcept;

GgError ggipc_update_state(GgComponentState state) noexcept;

GgError ggipc_restart_component(GgBuffer component_name);

typedef void GgIpcSubscribeToConfigurationUpdateCallback(
    void *ctx,
    GgBuffer component_name,
    GgList key_path,
    GgIpcSubscriptionHandle handle
);

GgError ggipc_subscribe_to_configuration_update(
    const GgBuffer *component_name,
    GgBufList key_path,
    GgIpcSubscribeToConfigurationUpdateCallback *callback,
    void *ctx,
    GgIpcSubscriptionHandle *handle
);
}

#endif
