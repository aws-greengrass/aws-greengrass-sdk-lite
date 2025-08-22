#ifndef GGL_OBJECT_HPP
#define GGL_OBJECT_HPP

#include "ggl/buffer.hpp"
#include "ggl/list.hpp"
#include "ggl/map.hpp"
#include "ggl/types.hpp"
#include "ggl/utility.hpp"
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <span>
#include <string_view>
#include <type_traits>
#include <variant>

namespace ggl {

class BadObjectAccess : public std::exception {
public:
    /// must be a string literal
    BadObjectAccess(const char *what) noexcept
        : m_what { what } {
    }

    const char *what() const noexcept override {
        return m_what;
    }

private:
    const char *m_what = "Bad ggl::Object access";
};

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

    Object() noexcept = default;

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

    explicit Object(Variant variant) noexcept
        : Object { to_object(variant) } {
    }

    constexpr Object(const Object &) noexcept = default;
    constexpr Object &operator=(const Object &) noexcept = default;

    explicit Object(std::monostate /*unused*/) noexcept { };

    explicit Object(::std::span<uint8_t> value) noexcept
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

    explicit Object(std::int64_t value) noexcept
        : GglObject { ggl_obj_i64(value) } {
    }

    explicit Object(double value) noexcept
        : GglObject { ggl_obj_f64(value) } {
    }

    explicit Object(const char *str) noexcept
        : Object { as_buffer(str) } {
    }

    template <class T> Object &operator=(const T &value) noexcept {
        static_assert(std::is_constructible_v<Object, T>);
        return *this = Object { value };
    }

    GglObjectType index() const noexcept {
        return ggl_obj_type(*this);
    }

    // Assumes Object is a Map
    Object operator[](std::string_view key) const {
        if (index() != GGL_TYPE_MAP) {
            GGL_THROW_OR_ABORT(BadObjectAccess {
                "key operator[]: ggl::Object is not ggl::Map" });
        }

        return *Map { ggl_obj_into_map(*this) }[key];
    }

    // Assumes Object is a List
    Object operator[](std::size_t idx) const {
        if (index() != GGL_TYPE_LIST) {
            GGL_THROW_OR_ABORT(BadObjectAccess {
                "index operator[]: ggl::Object is not ggl::List" });
        }
        return List { ggl_obj_into_list(*this) }[idx];
    }
};

static_assert(
    std::is_standard_layout_v<ggl::Object>,
    "ggl::Object must not declare any virtual functions or data members."
);

}

#endif
