#ifndef GGL_MAP_HPP
#define GGL_MAP_HPP

#include "ggl/buffer.hpp"
#include "ggl/types.hpp"
#include "ggl/utility.hpp"
#include <algorithm>
#include <cstdint>
#include <string_view>
#include <utility>
#include <stdexcept>

namespace ggl {

class Object;

struct KV : public GglKV {
public:
    KV() noexcept = default;

    constexpr KV(const KV &) noexcept = default;

    constexpr KV(const GglKV &kv) noexcept
        : GglKV { kv } {
    }

    constexpr KV &operator=(const KV &) noexcept = default;

    constexpr KV &operator=(const GglKV &kv) noexcept {
        return *this = KV { kv };
    }

    KV(GglBuffer key, GglObject value) noexcept
        : GglKV { ggl_kv(key, value) } {
    }

    KV(std::string_view key, GglObject value) noexcept
        : KV { as_buffer(key), value } {
    }

    // key must be null-terminated
    KV(const char *key, GglObject value) noexcept
        : KV { as_buffer(key), value } {
    }

    KV(std::span<uint8_t> key, GglObject value) noexcept
        : KV { as_buffer(key), value } {
    }

    std::string_view key() const noexcept {
        GglBuffer key = ggl_kv_key(*this);
        return std::string_view { reinterpret_cast<char *>(key.data), key.len };
    }

    Object *value() noexcept;

    const Object *value() const noexcept {
        return const_cast<KV *>(this)->value();
    }

    void key(std::string_view str) noexcept {
        ggl_kv_set_key(this, as_buffer(str));
    }

    void key(std::span<uint8_t> key) noexcept {
        ggl_kv_set_key(this, as_buffer(key));
    }

    void value(GglObject obj) noexcept {
        *ggl_kv_val(this) = obj;
    }
};

/// A pair-list of key-value pairs
class Map : public GglMap {
public:
    using key_type = std::string_view;
    using mapped_type = Object *;
    using value_type = std::pair<key_type, mapped_type>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    constexpr Map(KV *pair_array, size_type size) noexcept
        : GglMap { pair_array, size } {
    }

    constexpr Map(const GglMap &map) noexcept
        : GglMap { map } {
    }

    constexpr Map(const Map &other) noexcept = default;

    constexpr Map &operator=(const Map &other) noexcept = default;

    constexpr Map &operator=(const GglMap &other) noexcept {
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
            GGL_THROW_OR_ABORT(std::out_of_range {
                "ggl::Map::at: Non-existent key." });
        }
        return *(found->second);
    }

    // for mutating the pair-list e.g. map[0] = ggl::KV{"key", Object{}};
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

        constexpr difference_type operator-(const MapIterator &rhs
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

        constexpr bool operator==(const MapIterator &rhs) const noexcept {
            return kv == rhs.kv;
        }

        constexpr bool operator!=(const MapIterator &rhs) const noexcept {
            return !(*this == rhs);
        }

        constexpr bool operator<(const MapIterator &rhs) const noexcept {
            return kv < rhs.kv;
        }

        constexpr bool operator>=(const MapIterator &rhs) const noexcept {
            return !(*this < rhs);
        }

        constexpr bool operator>(const MapIterator &rhs) const noexcept {
            return rhs < *this;
        }

        constexpr bool operator<=(const MapIterator &rhs) const noexcept {
            return !(*this > rhs);
        }

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
}

static_assert(
    std::is_standard_layout_v<ggl::KV>,
    "ggl::KV must not declare any virtual functions or data members."
);

static_assert(
    std::is_standard_layout_v<ggl::Map>,
    "ggl::KV must not declare any virtual functions or data members."
);

#endif
