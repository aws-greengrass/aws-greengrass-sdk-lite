// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_MAP_HPP
#define GG_MAP_HPP

#include <gg/buffer.hpp>
#include <gg/types.hpp>
#include <gg/utility.hpp>
#include <stddef.h>
#include <cstdint>
#include <algorithm>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string> // IWYU pragma: keep (std::char_traits)
#include <string_view>
#include <type_traits>
#include <utility>

namespace gg {

class Object;

struct KV : public GgKV {
public:
    KV() noexcept = default;

    constexpr KV(const KV &) noexcept = default;

    constexpr KV(const GgKV &kv) noexcept
        : GgKV { kv } {
    }

    constexpr KV &operator=(const KV &) noexcept = default;

    constexpr KV &operator=(const GgKV &kv) noexcept {
        return *this = KV { kv };
    }

    KV(Buffer key, const Object &value) noexcept;

    KV(std::string_view key, const Object &value) noexcept
        : KV { Buffer { key }, value } {
    }

    KV(std::span<uint8_t> key, const Object &value) noexcept
        : KV { Buffer { key }, value } {
    }

    std::string_view key() const noexcept {
        GgBuffer key = gg_kv_key(*this);
        return std::string_view { reinterpret_cast<char *>(key.data), key.len };
    }

    Object *value() noexcept;

    const Object *value() const noexcept {
        return const_cast<KV *>(this)->value();
    }

    void key(std::string_view str) noexcept {
        gg_kv_set_key(this, Buffer { str });
    }

    void key(std::span<uint8_t> key) noexcept {
        gg_kv_set_key(this, Buffer { key });
    }

    void value(GgObject obj) noexcept {
        *gg_kv_val(this) = obj;
    }
};

/// A pair-list of key-value pairs
class Map : public GgMap {
public:
    using key_type = std::string_view;
    using mapped_type = Object *;
    using value_type = std::pair<key_type, mapped_type>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    constexpr Map() noexcept = default;

    constexpr Map(KV *pair_array, size_type size) noexcept
        : GgMap { pair_array, size } {
    }

    constexpr Map(const GgMap &map) noexcept
        : GgMap { map } {
    }

    constexpr Map(const Map &other) noexcept = default;

    constexpr Map &operator=(const Map &other) noexcept = default;

    constexpr Map &operator=(const GgMap &other) noexcept {
        return *this = Map { other };
    }

    mapped_type operator[](key_type key) const noexcept {
        auto found = find(key);
        if (found == cend()) {
            return nullptr;
        }
        return found->second;
    }

    Object &at(key_type key) const {
        auto found = find(key);
        if (found == cend()) {
            GG_THROW_OR_ABORT(
                std::out_of_range { "gg::Map::at: Non-existent key." }
            );
        }
        return *(found->second);
    }

    // for mutating the pair-list e.g. map[0] = gg::KV{"key", Object{}};
    constexpr KV &operator[](size_type idx) const noexcept {
        return data()[idx];
    }

    constexpr size_type size() const noexcept {
        return len;
    }

    constexpr KV *data() const noexcept {
        return static_cast<KV *>(pairs);
    }

    class MapIterator {
    private:
        KV *kv;

    public:
        using iterator_category = std::random_access_iterator_tag;

        using value_type = Map::value_type;
        using difference_type = ptrdiff_t;
        using distance_type = difference_type;

    private:
        struct ArrowProxy {
            // needs to be public to allow aggregate construction
            // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
            value_type value;

            constexpr value_type *operator->() noexcept {
                return &value;
            }
        };

    public:
        constexpr MapIterator(KV *pair) noexcept
            : kv { pair } {
        }

        constexpr MapIterator(const MapIterator &) noexcept = default;

        constexpr MapIterator &operator++() noexcept {
            ++kv;
            return *this;
        }

        constexpr MapIterator operator++(int) noexcept {
            MapIterator temp = *this;
            ++*this;
            return temp;
        }

        constexpr MapIterator &operator--() noexcept {
            --kv;
            return *this;
        }

        constexpr MapIterator operator--(int) noexcept {
            MapIterator temp = *this;
            --*this;
            return temp;
        }

        constexpr difference_type operator-(
            const MapIterator &rhs
        ) const noexcept {
            return std::distance(rhs.kv, kv);
        }

        constexpr MapIterator operator+(difference_type offset) const noexcept {
            MapIterator temp = *this;
            return temp += offset;
        }

        constexpr MapIterator operator-(difference_type offset) const noexcept {
            MapIterator temp = *this;
            return temp -= offset;
        }

        constexpr MapIterator &operator-=(difference_type offset) noexcept {
            kv = kv - offset;
            return *this;
        }

        friend constexpr MapIterator operator+(
            difference_type offset, const MapIterator &iter
        ) noexcept {
            return iter + offset;
        }

        constexpr MapIterator &operator+=(difference_type offset) noexcept {
            kv = &kv[offset];
            return *this;
        }

        value_type operator[](difference_type offset) const noexcept {
            return *(*this + offset);
        }

        constexpr bool operator<=>(const MapIterator &rhs) const noexcept
            = default;

        value_type operator*() const noexcept {
            return { kv->key(), kv->value() };
        }

        ArrowProxy operator->() const noexcept {
            return ArrowProxy { operator*() };
        }
    };

    using iterator = MapIterator;
    using const_iterator = iterator;

    constexpr iterator begin() const noexcept {
        return { data() };
    }

    constexpr iterator end() const noexcept {
        return { data() + size() };
    }

    constexpr const_iterator cbegin() const noexcept {
        return { data() };
    }

    constexpr const_iterator cend() const noexcept {
        return { data() + size() };
    }

    iterator find(key_type key) const noexcept {
        return std::find_if(begin(), end(), [key](value_type kv) {
            return kv.first == key;
        });
    }
};

} // namespace gg

static_assert(
    std::is_standard_layout_v<gg::KV>,
    "gg::KV must not declare any virtual functions or data members."
);

static_assert(
    std::is_standard_layout_v<gg::Map>,
    "gg::KV must not declare any virtual functions or data members."
);

#endif
