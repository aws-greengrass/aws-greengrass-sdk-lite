// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_OBJECT_HPP
#define GG_OBJECT_HPP

#include <gg/buffer.hpp>
#include <gg/error.hpp>
#include <gg/list.hpp>
#include <gg/map.hpp>
#include <gg/types.hpp>
#include <gg/utility.hpp>
#include <cstdint>
#include <cstdlib>
#include <concepts>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <variant>

namespace gg {

template <class T>
using is_object_alternative = std::disjunction<
    std::is_same<T, bool>,
    std::is_integral<T>,
    std::is_floating_point<T>,
    std::is_same<T, std::span<uint8_t>>,
    std::is_same<std::string_view, T>,
    std::is_same<T, List>,
    std::is_same<T, Map>>;

template <class T>
constexpr bool is_object_alternative_v = is_object_alternative<T>::value;

class [[nodiscard]] Object : public GgObject {
public:
    using Variant = ::std::variant<
        ::std::monostate,
        bool,
        ::std::int64_t,
        double,
        ::std::span<uint8_t>,
        ::gg::List,
        ::gg::Map>;

    constexpr Object() noexcept = default;

    constexpr Object(GgObject obj) noexcept
        : GgObject { obj } {
    }

    static Object to_object(const Variant &variant) {
        if (variant.valueless_by_exception()) {
            return {};
        }
        return std::visit(
            [](auto &&value) noexcept -> Object { return Object { value }; },
            variant
        );
    }

    Variant to_variant() const noexcept {
        switch (index()) {
        case GG_TYPE_NULL:
            return {};
        case GG_TYPE_BOOLEAN:
            return gg_obj_into_bool(*this);
        case GG_TYPE_I64:
            return gg_obj_into_i64(*this);
        case GG_TYPE_F64:
            return gg_obj_into_f64(*this);
        case GG_TYPE_BUF: {
            GgBuffer buf = gg_obj_into_buf(*this);
            return std::span { buf.data, buf.len };
        }
        case GG_TYPE_LIST:
            return gg_obj_into_list(*this);
        case GG_TYPE_MAP:
            return gg_obj_into_map(*this);
        }
        abort();
    }

    explicit Object(const Variant &variant) noexcept
        : Object { to_object(variant) } {
    }

    constexpr Object(const Object &) noexcept = default;
    constexpr Object &operator=(const Object &) noexcept = default;

    Object(std::monostate /*unused*/) noexcept { };

    explicit Object(bool boolean) noexcept
        : GgObject { gg_obj_bool(boolean) } {
    }

    Object(std::integral auto i64) noexcept
        requires(!std::is_same_v<bool, std::remove_cv_t<decltype(i64)>>)
        : GgObject { gg_obj_i64(static_cast<int64_t>(i64)) } {
    }

    Object(std::floating_point auto f64) noexcept
        : GgObject { gg_obj_f64(static_cast<double>(f64)) } {
    }

    Object(gg::Buffer buffer) noexcept
        : GgObject { gg_obj_buf(buffer) } {
    }

    Object(std::span<uint8_t> buffer) noexcept
        : Object { Buffer { buffer } } {
    }

    Object(std::string_view str) noexcept
        : Object { Buffer { str } } {
    }

    template <size_t N>
    Object(const char (&arr)[N]) noexcept
        : Object { std::string_view { arr } } {
    }

    Object(List list) noexcept
        : GgObject { gg_obj_list(list) } {
    }

    Object(Map map) noexcept
        : GgObject { gg_obj_map(map) } {
    }

    template <class T>
    Object &operator=(const T &value) noexcept
        requires(std::is_constructible_v<Object, T>)
    {
        return *this = Object { value };
    }

    GgObjectType index() const noexcept {
        return gg_obj_type(*this);
    }

    // Assumes Object is a Map
    Object operator[](std::string_view key) const {
        if (index() != GG_TYPE_MAP) {
            GG_THROW_OR_ABORT(
                Exception { GG_ERR_PARSE, "gg::Object is not gg::Map" }
            );
        }

        return *Map { gg_obj_into_map(*this) }[key];
    }

    // Assumes Object is a List
    Object operator[](std::size_t idx) const {
        if (index() != GG_TYPE_LIST) {
            GG_THROW_OR_ABORT(
                Exception { GG_ERR_PARSE, "gg::Object is not gg::List" }
            );
        }
        return List { gg_obj_into_list(*this) }[idx];
    }
};

template <class T>
concept ObjectType = std::conjunction_v<
    std::negation<std::is_pointer<T>>,
    std::is_constructible<Object, T>>;

template <class T>
using is_buffer_type = std::disjunction<
    std::is_same<T, Buffer>,
    std::is_same<T, std::span<uint8_t>>,
    std::is_same<T, std::string_view>>;

template <ObjectType T> constexpr GgObjectType index_for_type() noexcept {
    using Type = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<Type, bool>) {
        return GG_TYPE_BOOLEAN;
    } else if constexpr (std::is_integral_v<Type>) {
        return GG_TYPE_I64;
    } else if constexpr (std::is_floating_point_v<Type>) {
        return GG_TYPE_F64;
    } else if constexpr (is_buffer_type<Type>::value) {
        return GG_TYPE_BUF;
    } else if constexpr (std::is_same_v<Type, List>) {
        return GG_TYPE_LIST;
    } else if constexpr (std::is_same_v<Type, Map>) {
        return GG_TYPE_MAP;
    } else {
        return GG_TYPE_NULL;
    }
}

template <ObjectType T> std::optional<T> get_if(Object *obj) noexcept {
    using Type = std::remove_cvref_t<T>;
    if (obj == nullptr) {
        return std::nullopt;
    }
    if (obj->index() != index_for_type<Type>()) {
        return std::nullopt;
    }
    if constexpr (std::is_same_v<Type, bool>) {
        return gg_obj_into_bool(*obj);
    } else if constexpr (std::is_integral_v<Type>) {
        return gg_obj_into_i64(*obj);
    } else if constexpr (std::is_floating_point_v<Type>) {
        return gg_obj_into_f64(*obj);
    } else if constexpr (is_buffer_type<Type>::value) {
        Buffer buf = gg_obj_into_buf(*obj);
        return T { reinterpret_cast<typename T::pointer>(buf.data()),
                   buf.size() };
    } else if constexpr (std::is_same_v<Type, List>) {
        return gg_obj_into_list(*obj);
    } else if constexpr (std::is_same_v<Type, Map>) {
        return gg_obj_into_map(*obj);
    } else {
        return T {};
    }
}

template <ObjectType T> T get(Object obj) {
    auto value = get_if<T>(&obj);
    if (!value) {
        GG_THROW_OR_ABORT(
            Exception { GG_ERR_PARSE, "get: Bad index for object." }
        );
    }
    return *value;
}

constexpr Object nullobj {};

static_assert(
    std::is_standard_layout_v<gg::Object>,
    "gg::Object must not declare any virtual functions or data members."
);
}

#endif
