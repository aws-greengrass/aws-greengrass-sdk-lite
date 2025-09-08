#ifndef GGL_SCHEMA_HPP
#define GGL_SCHEMA_HPP

#include "ggl/error.hpp"
#include "ggl/map.hpp"
#include "ggl/object.hpp"
#include <ggl/types.hpp>
#include <optional>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <variant>

namespace ggl {

class MissingKey { };

template <class T> struct is_map_schema_candidate {
    static constexpr bool value = std::disjunction_v<
        is_object_alternative<T>,
        std::is_same<Object, T>,
        std::is_same<MissingKey, T>>;
};

template <class T> struct is_map_schema_candidate<std::optional<T>> {
    static constexpr bool value = std::disjunction_v<
        is_object_alternative<T>,
        std::is_same<std::optional<Object>, std::optional<T>>>;
};

template <class T>
concept MapSchemaType = is_map_schema_candidate<T>::value;

template <MapSchemaType T> class MapSchema {
    std::string_view key;
    Object **object;
    T *entry;

public:
    constexpr MapSchema(
        std::string_view key, T &entry, Object *&object
    ) noexcept
        : key { key }
        , object { &object }
        , entry { &entry } {
    }

    constexpr MapSchema(std::string_view key, T &entry) noexcept
        : key { key }
        , object { nullptr }
        , entry { &entry } {
    }

    std::error_code validate(const Map &map) noexcept {
        auto found = map.find(key);

        if (found == map.cend()) {
            return GGL_ERR_NOENTRY;
        }

        if constexpr (std::is_same_v<T, Object>) {
            *entry = *(found->second);
        } else {
            auto variant = found->second->to_variant();
            auto *value = std::get_if<T>(&variant);
            if (value == nullptr) {
                return GGL_ERR_PARSE;
            }
            *entry = *value;
        }

        if (object != nullptr) {
            *object = found->second;
        }

        return GGL_ERR_OK;
    }
};

template <MapSchemaType T> class MapSchema<std::optional<T>> {
    std::string_view key;
    Object **object;
    std::optional<T> *entry;

public:
    constexpr MapSchema(
        std::string_view key, std::optional<T> &entry, Object *&object
    ) noexcept
        : key { key }
        , object { &object }
        , entry { &entry } {
    }

    constexpr MapSchema(std::string_view key, std::optional<T> &entry) noexcept
        : key { key }
        , object { nullptr }
        , entry { &entry } {
    }

    std::error_code validate(const Map &map) const noexcept {
        auto found = map.find(key);

        if (found == map.cend()) {
            if (object != nullptr) {
                *object = nullptr;
            }
            *entry = std::nullopt;
            return GGL_ERR_OK;
        }

        if constexpr (std::is_same_v<T, Object>) {
            *entry = *found->second;
        } else {
            auto variant = found->second->to_variant();
            auto *got = std::get_if<T>(&variant);
            if (got == nullptr) {
                *entry = std::nullopt;
            } else {
                *entry = *got;
            }
        }

        if (object != nullptr) {
            *object = found->second;
        }

        return GGL_ERR_OK;
    }
};

template <> class MapSchema<MissingKey> {
    std::string_view key;

public:
    constexpr MapSchema(std::string_view key) noexcept
        : key { key } {
    }

    std::error_code validate(const Map &map) const noexcept {
        auto found = map.find(key);
        if (found == map.cend()) {
            return GGL_ERR_OK;
        }
        return GGL_ERR_PARSE;
    }
};

using MapSchemaMissingEntry = MapSchema<MissingKey>;

template <class T>
MapSchema(std::string_view, std::optional<T> &, Object *&)
    -> MapSchema<std::optional<T>>;

template <class T>
MapSchema(std::string_view, std::optional<T> &) -> MapSchema<std::optional<T>>;

MapSchema(std::string_view, Object *&) -> MapSchema<Object>;

MapSchema(std::string_view) -> MapSchema<MissingKey>;

template <class... Ts, MapSchema<Ts>...>
std::error_code validate_map(
    const Map &map, MapSchema<Ts> &&...schemas
) noexcept {
    std::error_code result = GGL_ERR_OK;
    (void) (((schemas.validate(map)) ? (result = schemas.validate(map), true)
                                     : false)
            || ...);
    return result;
}

}

#endif
