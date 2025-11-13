// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_IPC_CLIENT_HPP
#define GG_IPC_CLIENT_HPP

#include <gg/buffer.hpp>
#include <gg/list.hpp>
#include <gg/map.hpp>
#include <gg/object.hpp>
#include <gg/sdk.hpp>
#include <gg/types.hpp>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace gg::ipc {

class Subscription;

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

class AuthToken {
    std::string_view token;

public:
    constexpr AuthToken() noexcept = default;

    explicit constexpr AuthToken(std::string_view token) noexcept
        : token { token } { };

    explicit operator Buffer() const noexcept {
        return Buffer { token };
    }

    static std::optional<AuthToken> from_environment() noexcept;
};

class LocalTopicCallback {
public:
    virtual ~LocalTopicCallback() noexcept = default;
    virtual void operator()(
        std::string_view topic, gg::Object payload, Subscription &handle
    ) = 0;
};

class IotTopicCallback {
public:
    virtual ~IotTopicCallback() noexcept = default;
    virtual void operator()(
        std::string_view topic, gg::Buffer payload, Subscription &handle
    ) = 0;
};

class ConfigurationUpdateCallback {
public:
    virtual ~ConfigurationUpdateCallback() noexcept = default;
    virtual void operator()(
        std::string_view component_name, gg::List key_path, Subscription &handle
    ) = 0;
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
        std::string_view socket_path, AuthToken auth_token
    ) noexcept;

    std::error_code publish_to_topic(
        std::string_view topic, Buffer bytes
    ) noexcept;

    std::error_code publish_to_topic(
        std::string_view topic, const Map &json
    ) noexcept;

    std::error_code subscribe_to_topic(
        std::string_view topic,
        LocalTopicCallback &callback,
        Subscription *handle = nullptr
    ) noexcept;

    std::error_code publish_to_iot_core(
        std::string_view topic, Buffer bytes, uint8_t qos
    ) noexcept;

    std::error_code subscribe_to_iot_core(
        std::string_view topic_filter,
        std::uint8_t qos,
        IotTopicCallback &callback,
        Subscription *handle = nullptr
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

    std::error_code get_config(
        std::span<const Buffer> key_path,
        std::optional<std::string_view> component_name,
        std::int64_t &value
    ) noexcept;

    std::error_code get_config(
        std::span<const Buffer> key_path,
        std::optional<std::string_view> component_name,
        double &value
    ) noexcept;

    std::error_code get_config(
        std::span<const Buffer> key_path,
        std::optional<std::string_view> component_name,
        bool &value
    ) noexcept;

    std::error_code subscribe_to_configuration_update(
        std::span<const Buffer> key_path,
        std::optional<std::string_view> component_name,
        ConfigurationUpdateCallback &callback,
        Subscription *handle = nullptr
    ) noexcept;
};

}

#endif
