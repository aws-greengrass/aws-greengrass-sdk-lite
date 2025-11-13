// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_SCHEMA_HPP
#define GG_SCHEMA_HPP

#include <gg/error.hpp>
#include <gg/map.hpp>
#include <gg/object.hpp>
#include <optional>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace gg {

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

// code deduplication for Optional and non-Optional schema
template <class T> class MapSchemaBase {
protected:
    static GgError retrieve_value(
        Map::iterator iter, Object **object, T &entry
    ) noexcept {
        if constexpr (std::is_same_v<T, Object>) {
            entry = *iter->second;
        } else {
            auto value = get_if<T>(iter->second);
            if (!value) {
                return GG_ERR_PARSE;
            }
            entry = *value;
        }

        if (object != nullptr) {
            *object = iter->second;
        }

        return GG_ERR_OK;
    }
};

template <MapSchemaType T> class MapSchema : MapSchemaBase<T> {
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
            return GG_ERR_NOENTRY;
        }

        return MapSchemaBase<T>::retrieve_value(found, object, *entry);
    }
};

template <MapSchemaType T>
class MapSchema<std::optional<T>> : MapSchemaBase<T> {
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
            return GG_ERR_OK;
        }

        T val;
        GgError ret = MapSchemaBase<T>::retrieve_value(found, object, val);
        if (ret == GG_ERR_OK) {
            *entry = val;
        }
        return ret;
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
            return GG_ERR_OK;
        }
        return GG_ERR_PARSE;
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
    std::error_code result = GG_ERR_OK;
    (void) (((schemas.validate(map)) ? (result = schemas.validate(map), true)
                                     : false)
            || ...);
    return result;
}

}

#endif
