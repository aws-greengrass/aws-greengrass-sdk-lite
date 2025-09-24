// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_CLIENT_HPP
#define GGL_IPC_CLIENT_HPP

#include <ggl/buffer.hpp>
#include <ggl/map.hpp>
#include <ggl/object.hpp>
#include <ggl/sdk.hpp>
#include <ggl/types.hpp>
#include <cstdint>
#include <chrono>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>

namespace ggl::ipc {

/// Heap-allocated object. The result of some IPC operations.
class AllocatedObject {
private:
    std::unique_ptr<std::byte[]> arena;
    Object head;

public:
    constexpr AllocatedObject() noexcept = default;

    AllocatedObject(
        const Object &head, std::unique_ptr<std::byte[]> &&arena
    ) noexcept
        : arena { std::move(arena) }
        , head { head } {
    }

    AllocatedObject &operator=(AllocatedObject &&) noexcept = default;
    AllocatedObject(AllocatedObject &&) noexcept = default;

    constexpr Object get() const noexcept {
        return head;
    }
};

class Client {
private:
    constexpr Client() noexcept = default;

public:
    static Client &get() noexcept {
        (void) Sdk::get();
        static Client singleton {};
        return singleton;
    }

    // Thread-safe as long as no other thread modifies environment variables
    std::error_code connect() noexcept;

    std::error_code connect(
        std::string_view socket_path, std::string_view auth_token
    ) noexcept;

    std::error_code publish_to_topic(
        std::string_view topic, Buffer bytes
    ) noexcept;

    std::error_code publish_to_topic(
        std::string_view topic, const Map &json
    ) noexcept;

    std::error_code publish_to_iot_core(
        std::string_view topic, Buffer bytes, uint8_t qos
    ) noexcept;

    std::error_code update_config(
        std::span<const Buffer> key_path,
        const Object &value,
        std::chrono::system_clock::time_point timestamp = {}
    ) noexcept;

    std::error_code update_component_state(ComponentState state) noexcept;

    std::error_code restart_component(std::string_view component_name) noexcept;

    std::error_code get_config(
        std::span<const Buffer> key_path,
        std::optional<std::string_view> component_name,
        std::string &value
    ) noexcept;

    std::error_code get_config(
        std::span<const Buffer> key_path,
        std::optional<std::string_view> component_name,
        AllocatedObject &value
    ) noexcept;
};

}

#endif
