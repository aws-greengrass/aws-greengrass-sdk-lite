// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/buffer.hpp>
#include <gg/error.hpp>
#include <gg/ipc/client.hpp>
#include <gg/ipc/client_c_api.hpp>
#include <gg/map.hpp>
#include <stdint.h>
#include <cstdlib>
#include <optional>
#include <string_view>
#include <system_error>

enum class GgComponentState;

namespace gg::ipc {

std::optional<AuthToken> AuthToken::from_environment() noexcept {
    // C++ std::getenv has stronger thread-safety guarantees than C's
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    char *svcuid = std::getenv("SVCUID");
    // std::string_view nullptr ctor is UB (deleted in C++23)
    if (svcuid == nullptr) {
        return std::nullopt;
    }
    return AuthToken { svcuid };
}

std::error_code Client::connect() noexcept {
    auto auth_token = AuthToken::from_environment();
    if (!auth_token.has_value()) {
        return GG_ERR_CONFIG;
    }

    char *socket_path
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        = std::getenv("AWS_GG_NUCLEUS_DOMAIN_SOCKET_FILEPATH_FOR_COMPONENT");
    if (socket_path == nullptr) {
        return GG_ERR_CONFIG;
    }

    return connect(socket_path, *auth_token);
}

// singleton interface class.
// NOLINTBEGIN(readability-convert-member-functions-to-static)

std::error_code Client::connect(
    std::string_view socket_path, AuthToken auth_token
) noexcept {
    return ggipc_connect_with_token(
        Buffer { socket_path }, Buffer { auth_token }
    );
}

std::error_code Client::update_component_state(
    GgComponentState state
) noexcept {
    return ggipc_update_state(state);
}

std::error_code Client::publish_to_topic(
    std::string_view topic, Buffer bytes
) noexcept {
    return ggipc_publish_to_topic_binary(Buffer { topic }, bytes);
}

std::error_code Client::publish_to_topic(
    std::string_view topic, const Map &json
) noexcept {
    return ggipc_publish_to_topic_json(Buffer { topic }, json);
}

std::error_code Client::publish_to_iot_core(
    std::string_view topic, Buffer bytes, uint8_t qos
) noexcept {
    return ggipc_publish_to_iot_core(Buffer { topic }, bytes, qos);
}

std::error_code Client::restart_component(
    std::string_view component_name
) noexcept {
    return ggipc_restart_component(Buffer { component_name });
}

// NOLINTEND(readability-convert-member-functions-to-static)

}
