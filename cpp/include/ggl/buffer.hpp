#ifndef GGL_BUFFER_HPP
#define GGL_BUFFER_HPP

#include "ggl/types.hpp"
#include "ggl/utility.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <stdexcept>

namespace ggl {

class Buffer : public GglBuffer {
public:
    using element_type = std::remove_pointer_t<decltype(GglBuffer::data)>;
    using reference = element_type &;
    using pointer = element_type *;
    using iterator = pointer;
    using size_type = decltype(GglBuffer::len);

    constexpr Buffer() noexcept = default;
    constexpr Buffer(const Buffer &) noexcept = default;

    constexpr Buffer(const GglBuffer &buffer) noexcept
        : GglBuffer { buffer } {
    }

    constexpr Buffer(pointer buf, size_type size) noexcept
        : GglBuffer { buf, size } {
    }

    Buffer(const char *str) noexcept;

    constexpr Buffer &operator=(const Buffer &) noexcept = default;

    constexpr Buffer &operator=(const GglBuffer &buffer) noexcept {
        return *this = Buffer { buffer };
    }

    constexpr pointer data() const noexcept {
        return GglBuffer::data;
    }

    constexpr size_type size() const noexcept {
        return len;
    }

    constexpr bool empty() const noexcept {
        return size() == size_type(0);
    }

    constexpr reference operator[](size_type idx) const noexcept {
        return data()[idx];
    }

    constexpr reference at(size_type idx) const {
        if (idx >= size()) {
            GGL_THROW_OR_ABORT(std::out_of_range { "Buffer:at" });
        }
        return data()[idx];
    }

    constexpr iterator begin() const noexcept {
        return data();
    }

    constexpr iterator end() const noexcept {
        return data() + size();
    }

    constexpr iterator cbegin() const noexcept {
        return data();
    }

    constexpr iterator cend() const noexcept {
        return data() + size();
    }
};

template <class T> Buffer as_buffer(std::span<T> bytes) noexcept {
    return Buffer { reinterpret_cast<std::uint8_t *>(bytes.data()),
                    bytes.size_bytes() };
}

template <> constexpr Buffer as_buffer(std::span<std::uint8_t> bytes) noexcept {
    return Buffer { bytes.data(), bytes.size_bytes() };
}

template <class Traits = std::char_traits<std::uint8_t>>
constexpr Buffer as_buffer(std::basic_string_view<std::uint8_t, Traits> sv
) noexcept {
    return Buffer { const_cast<std::uint8_t *>(sv.data()), sv.size() };
}

template <class CharT, class Traits = std::char_traits<CharT>>
Buffer as_buffer(std::basic_string_view<CharT, Traits> sv) noexcept {
    auto *data
        = reinterpret_cast<std::uint8_t *>(const_cast<CharT *>(sv.data()));
    if constexpr (sizeof(CharT) > 1) {
        return Buffer { data, sv.size() / sizeof(CharT) };
    } else {
        return Buffer { data, sv.size() };
    }
}

/// Must be string literal / c string
inline Buffer as_buffer(const char *str) noexcept {
    return as_buffer(std::string_view { str });
}

inline Buffer::Buffer(const char *str) noexcept
    : GglBuffer { as_buffer(std::string_view { str }) } {
}

}

#endif
