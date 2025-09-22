#ifndef GGL_OBJECT_HPP
#define GGL_OBJECT_HPP

#include <ggl/buffer.hpp>
#include <ggl/error.hpp>
#include <ggl/list.hpp>
#include <ggl/map.hpp>
#include <ggl/types.hpp>
#include <ggl/utility.hpp>
#include <cstdint>
#include <cstdlib>
#include <concepts>
#include <span>
#include <string_view>
#include <type_traits>
#include <variant>

namespace ggl {

template <class T>
using is_object_alternative = std::disjunction<
    std::is_same<T, bool>,
    std::is_integral<T>,
    std::is_floating_point<T>,
    std::is_same<T, std::span<uint8_t>>,
    std::is_same<T, List>,
    std::is_same<T, Map>>;

template <class T>
constexpr bool is_object_alternative_v = is_object_alternative<T>::value;

class [[nodiscard]] Object : public GglObject {
public:
    using Variant = ::std::variant<
        ::std::monostate,
        bool,
        ::std::int64_t,
        double,
        ::std::span<uint8_t>,
        ::ggl::List,
        ::ggl::Map>;

    constexpr Object() noexcept = default;

    constexpr Object(GglObject obj) noexcept
        : GglObject { obj } {
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
        case GGL_TYPE_NULL:
            return {};
        case GGL_TYPE_BOOLEAN:
            return ggl_obj_into_bool(*this);
        case GGL_TYPE_I64:
            return ggl_obj_into_i64(*this);
        case GGL_TYPE_F64:
            return ggl_obj_into_f64(*this);
        case GGL_TYPE_BUF: {
            GglBuffer buf = ggl_obj_into_buf(*this);
            return std::span { buf.data, buf.len };
        }
        case GGL_TYPE_LIST:
            return ggl_obj_into_list(*this);
        case GGL_TYPE_MAP:
            return ggl_obj_into_map(*this);
        }
        abort();
    }

    explicit Object(const Variant &variant) noexcept
        : Object { to_object(variant) } {
    }

    constexpr Object(const Object &) noexcept = default;
    constexpr Object &operator=(const Object &) noexcept = default;

    explicit Object(std::monostate /*unused*/) noexcept { };

    explicit Object(std::span<uint8_t> value) noexcept
        : GglObject { ggl_obj_buf(as_buffer(value)) } {
    }

    explicit Object(ggl::Buffer value) noexcept
        : GglObject { ggl_obj_buf(value) } {
    }

    explicit Object(List value) noexcept
        : GglObject { ggl_obj_list(value) } {
    }

    explicit Object(Map value) noexcept
        : GglObject { ggl_obj_map(value) } {
    }

    explicit Object(bool value) noexcept
        : GglObject { ggl_obj_bool(value) } {
    }

    explicit Object(std::integral auto value) noexcept
        : GglObject { ggl_obj_i64(static_cast<int64_t>(value)) } {
    }

    explicit Object(std::floating_point auto value) noexcept
        : GglObject { ggl_obj_f64(static_cast<double>(value)) } {
    }

    explicit Object(const char *str) noexcept
        : Object { as_buffer(str) } {
    }

    explicit Object(std::string_view str) noexcept
        : Object { as_buffer(str) } {
    }

    template <class T>
    Object &operator=(const T &value) noexcept
        requires(std::is_constructible_v<Object, T>)
    {
        return *this = Object { value };
    }

    GglObjectType index() const noexcept {
        return ggl_obj_type(*this);
    }

    // Assumes Object is a Map
    Object operator[](std::string_view key) const {
        if (index() != GGL_TYPE_MAP) {
            GGL_THROW_OR_ABORT(
                Exception { GGL_ERR_PARSE, "ggl::Object is not ggl::Map" }
            );
        }

        return *Map { ggl_obj_into_map(*this) }[key];
    }

    // Assumes Object is a List
    Object operator[](std::size_t idx) const {
        if (index() != GGL_TYPE_LIST) {
            GGL_THROW_OR_ABORT(
                Exception { GGL_ERR_PARSE, "ggl::Object is not ggl::List" }
            );
        }
        return List { ggl_obj_into_list(*this) }[idx];
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

template <ObjectType T> constexpr GglObjectType index_for_type() noexcept {
    using Type = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<Type, bool>) {
        return GGL_TYPE_BOOLEAN;
    } else if constexpr (std::is_integral_v<Type>) {
        return GGL_TYPE_I64;
    } else if constexpr (std::is_floating_point_v<Type>) {
        return GGL_TYPE_F64;
    } else if constexpr (is_buffer_type<Type>::value) {
        return GGL_TYPE_BUF;
    } else if constexpr (std::is_same_v<Type, List>) {
        return GGL_TYPE_LIST;
    } else if constexpr (std::is_same_v<Type, Map>) {
        return GGL_TYPE_MAP;
    } else {
        return GGL_TYPE_NULL;
    }
}

template <ObjectType T> T get(Object obj) {
    using Type = std::remove_cvref_t<T>;
    if (obj.index() != index_for_type<Type>()) {
        GGL_THROW_OR_ABORT(
            Exception { GGL_ERR_PARSE, "get: Bad index for object." }
        );
    }
    if constexpr (std::is_same_v<Type, bool>) {
        return ggl_obj_into_bool(obj);
    } else if constexpr (std::is_integral_v<Type>) {
        return ggl_obj_into_i64(obj);
    } else if constexpr (std::is_floating_point_v<Type>) {
        return ggl_obj_into_f64(obj);
    } else if constexpr (is_buffer_type<Type>::value) {
        Buffer buf = ggl_obj_into_buf(obj);
        if constexpr (std::is_same_v<
                          Buffer::pointer,
                          std::remove_cv_t<typename T::pointer>>) {
            return { buf.data(), buf.size() };
        } else {
            return { reinterpret_cast<typename T::pointer>(buf.data()),
                     buf.size() };
        }
    } else if constexpr (std::is_same_v<Type, List>) {
        return ggl_obj_into_list(obj);
    } else if constexpr (std::is_same_v<Type, Map>) {
        return ggl_obj_into_map(obj);
    } else {
        return {};
    }
}

constexpr Object nullobj {};

static_assert(
    std::is_standard_layout_v<ggl::Object>,
    "ggl::Object must not declare any virtual functions or data members."
);
}

#endif
