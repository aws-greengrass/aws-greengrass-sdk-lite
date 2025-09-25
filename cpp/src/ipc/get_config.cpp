// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/arena.hpp>
#include <ggl/buffer.hpp>
#include <ggl/error.hpp>
#include <ggl/ipc/client.hpp>
#include <ggl/ipc/client_raw_c_api.hpp>
#include <ggl/list.hpp>
#include <ggl/map.hpp>
#include <ggl/object.hpp>
#include <ggl/schema.hpp>
#include <ggl/types.hpp>
#include <cstddef>
#include <algorithm>
#include <array>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace ggl::ipc::detail {

constexpr size_t ggl_max_object_depth = 15U;

extern "C" {
GglError ggl_arena_claim_obj(GglObject *obj, GglArena *arena) noexcept;

GglError get_config_str_callback(void *ctx, GglMap result) noexcept {
    std::string &str = *static_cast<std::string *>(ctx);

    std::string_view value;
    std::error_code error = validate_map(result, MapSchema { "value", value });
    if (error) {
        return GGL_ERR_PARSE;
    }

    try {
        str = value;
    } catch (...) {
        return GGL_ERR_NOMEM;
    }

    return GGL_ERR_OK;
}

GglError get_config_obj_callback(void *ctx, GglMap result) noexcept {
    AllocatedObject &obj = *static_cast<AllocatedObject *>(ctx);

    Object value;
    std::error_code error = validate_map(result, MapSchema { "value", value });
    if (error) {
        return GGL_ERR_PARSE;
    }

    size_t len;
    error = ggl_obj_mem_usage(value, &len);
    if (error) {
        return GGL_ERR_INVALID;
    }

    if (len == 0) {
        obj = AllocatedObject { value, nullptr };
        return GGL_ERR_OK;
    }

    std::unique_ptr<std::byte[]> alloc { new (std::nothrow) std::byte[len] };
    if (alloc == nullptr) {
        return GGL_ERR_NOMEM;
    }

    Arena arena { alloc.get(), len };
    error = ggl_arena_claim_obj(&value, &arena);
    [[unlikely]] // Object traversal errors won't occur
    if (error) {
        return GGL_ERR_NOMEM;
    }
    obj = AllocatedObject { value, std::move(alloc) };
    return GGL_ERR_OK;
}

GglError get_config_error_callback(
    [[maybe_unused]] void *ctx,
    GglBuffer error_code,
    [[maybe_unused]] GglBuffer message
) noexcept {
    if (Buffer { "ResourceNotFoundError" } == error_code) {
        return GGL_ERR_NOENTRY;
    }
    return GGL_ERR_FAILURE;
}
}

namespace {
    GglError get_config_common(
        std::span<const Buffer> key_path,
        std::optional<std::string_view> component_name,
        void *value,
        GgIpcResultCallback fn
    ) {
        if (key_path.size() > ggl_max_object_depth) {
            return GGL_ERR_NOMEM;
        }

        std::array<Object, ggl_max_object_depth> key_vec;
        std::ranges::copy(key_path, key_vec.begin());
        std::array param_pairs {
            KV { "keyPath", List { key_vec.data(), key_path.size() } },
            KV { "componentName", component_name.value_or("") }
        };

        size_t param_len
            = param_pairs.size() - (component_name.has_value() ? 0 : 1);

        Map params { param_pairs.data(), param_len };

        return ggipc_call(
            Buffer { "aws.greengrass#GetConfiguration" },
            Buffer { "aws.greengrass#GetConfigurationRequest" },
            params,
            fn,
            detail::get_config_error_callback,
            &value
        );
    }

}
}

namespace ggl::ipc {
// singleton interface class.
// NOLINTBEGIN(readability-convert-member-functions-to-static)

std::error_code Client::get_config(
    std::span<const Buffer> key_path,
    std::optional<std::string_view> component_name,
    std::string &value
) noexcept {
    return detail::get_config_common(
        key_path, component_name, &value, detail::get_config_str_callback
    );
}

std::error_code Client::get_config(
    std::span<const Buffer> key_path,
    std::optional<std::string_view> component_name,
    AllocatedObject &value
) noexcept {
    return detail::get_config_common(
        key_path, component_name, &value, detail::get_config_obj_callback
    );
}

// NOLINTEND(readability-convert-member-functions-to-static)
}
