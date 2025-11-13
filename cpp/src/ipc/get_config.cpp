// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/arena.hpp>
#include <gg/buffer.hpp>
#include <gg/error.hpp>
#include <gg/ipc/client.hpp>
#include <gg/ipc/client_raw_c_api.hpp>
#include <gg/list.hpp>
#include <gg/map.hpp>
#include <gg/object.hpp>
#include <gg/schema.hpp>
#include <gg/types.hpp>
#include <cstddef>
#include <cstdint>
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

namespace gg::ipc::detail {

constexpr size_t gg_max_object_depth = 15U;

extern "C" {

struct GetConfigObjectContext {
    AllocatedObject *obj;
    GgObjectType expected_type;
};

GgError gg_arena_claim_obj(GgObject *obj, GgArena *arena) noexcept;

GgError get_config_str_callback(void *ctx, GgMap result) noexcept {
    std::string &str = *static_cast<std::string *>(ctx);

    std::string_view value;
    std::error_code error = validate_map(result, MapSchema { "value", value });
    if (error) {
        return GG_ERR_PARSE;
    }

    try {
        str = value;
    } catch (...) {
        return GG_ERR_NOMEM;
    }

    return GG_ERR_OK;
}

GgError get_config_obj_callback(void *ctx, GgMap result) noexcept {
    auto &context = *static_cast<GetConfigObjectContext *>(ctx);

    Object value;
    std::error_code error = validate_map(result, MapSchema { "value", value });
    if (error) {
        return GG_ERR_PARSE;
    }
    if ((context.expected_type != GG_TYPE_NULL)
        && (context.expected_type != value.index())) {
        return GG_ERR_PARSE;
    }

    size_t len = 0;
    error = gg_obj_mem_usage(value, &len);
    if (error) {
        return GG_ERR_INVALID;
    }

    if (len == 0) {
        *context.obj = AllocatedObject { value, nullptr };
        return GG_ERR_OK;
    }

    std::unique_ptr<std::byte[]> alloc { new (std::nothrow) std::byte[len] };
    if (alloc == nullptr) {
        return GG_ERR_NOMEM;
    }

    Arena arena { alloc.get(), len };
    error = gg_arena_claim_obj(&value, &arena);
    [[unlikely]] // Object traversal errors won't occur
    if (error) {
        return GG_ERR_NOMEM;
    }
    *context.obj = AllocatedObject { value, std::move(alloc) };
    return GG_ERR_OK;
}

GgError get_config_error_callback(
    [[maybe_unused]] void *ctx,
    GgBuffer error_code,
    [[maybe_unused]] GgBuffer message
) noexcept {
    if (Buffer { "ResourceNotFoundError" } == error_code) {
        return GG_ERR_NOENTRY;
    }
    return GG_ERR_FAILURE;
}
}

namespace {
    GgError get_config_common(
        std::span<const Buffer> key_path,
        std::optional<std::string_view> component_name,
        void *value,
        GgIpcResultCallback fn
    ) {
        if (key_path.size() > gg_max_object_depth) {
            return GG_ERR_NOMEM;
        }

        std::array<Object, gg_max_object_depth> key_vec;
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
            value
        );
    }

}
}

namespace gg::ipc {
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
    detail::GetConfigObjectContext ctx { .obj = &value,
                                         .expected_type = GG_TYPE_NULL };
    return detail::get_config_common(
        key_path, component_name, &ctx, detail::get_config_obj_callback
    );
}

namespace {
    template <class T>
    std::error_code get_config_overload(
        std::span<const Buffer> key_path,
        std::optional<std::string_view> component_name,
        T &value
    ) noexcept {
        constexpr auto expected_type = gg::index_for_type<T>();
        AllocatedObject alloc {};
        detail::GetConfigObjectContext ctx { .obj = &alloc,
                                             .expected_type = expected_type };
        GgError error = detail::get_config_common(
            key_path, component_name, &ctx, detail::get_config_obj_callback
        );
        if (Object obj = alloc.get();
            (error == GG_ERR_OK) && (obj.index() == expected_type)) {
            value = *get_if<T>(&obj);
        }
        return error;
    }
}

std::error_code Client::get_config(
    std::span<const Buffer> key_path,
    std::optional<std::string_view> component_name,
    std::int64_t &value
) noexcept {
    return get_config_overload<std::int64_t>(key_path, component_name, value);
}

std::error_code Client::get_config(
    std::span<const Buffer> key_path,
    std::optional<std::string_view> component_name,
    double &value
) noexcept {
    return get_config_overload<double>(key_path, component_name, value);
}

std::error_code Client::get_config(
    std::span<const Buffer> key_path,
    std::optional<std::string_view> component_name,
    bool &value
) noexcept {
    return get_config_overload<bool>(key_path, component_name, value);
}

// NOLINTEND(readability-convert-member-functions-to-static)
}
