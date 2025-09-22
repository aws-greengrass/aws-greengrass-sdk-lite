#include "ggl/ipc/client.hpp"
#include "ggl/buffer.hpp"
#include "ggl/error.hpp"
#include "ggl/ipc/client_c_api.hpp"
#include "ggl/map.hpp"
#include "ggl/types.hpp"
#include <cstdlib>
#include <string_view>
#include <system_error>

namespace ggl::ipc {

std::error_code Client::connect() noexcept {
    // C++ std::getenv has stronger thread-safety guarantees than C's
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    char *svcuid = std::getenv("SVCUID");
    if (svcuid == NULL) {
        return GGL_ERR_CONFIG;
    }

    char *socket_path
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        = std::getenv("AWS_GG_NUCLEUS_DOMAIN_SOCKET_FILEPATH_FOR_COMPONENT");

    if (socket_path == NULL) {
        return GGL_ERR_CONFIG;
    }

    return connect(svcuid, socket_path);
}

// singleton interface class.
// NOLINTBEGIN(readability-convert-member-functions-to-static)

std::error_code Client::connect(
    std::string_view socket_path, std::string_view auth_token
) noexcept {
    return ggipc_connect_with_token(
        as_buffer(socket_path), as_buffer(auth_token)
    );
}

std::error_code Client::update_component_state(
    GglComponentState state
) noexcept {
    return ggipc_update_state(state);
}

std::error_code Client::publish_to_topic(
    std::string_view topic, Buffer bytes
) noexcept {
    return ggipc_publish_to_topic_binary(as_buffer(topic), bytes);
}

std::error_code Client::publish_to_topic(
    std::string_view topic, const Map &json
) noexcept {
    return ggipc_publish_to_topic_json(as_buffer(topic), json);
}

std::error_code Client::publish_to_iot_core(
    std::string_view topic, Buffer bytes, uint8_t qos
) noexcept {
    return ggipc_publish_to_iot_core(as_buffer(topic), bytes, qos);
}

std::error_code Client::restart_component(
    std::string_view component_name
) noexcept {
    return ggipc_restart_component(as_buffer(component_name));
}

// NOLINTEND(readability-convert-member-functions-to-static)

}
