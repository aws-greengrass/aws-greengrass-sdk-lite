// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_BUFFER_HPP
#define GGL_BUFFER_HPP

#include <ggl/types.hpp>
#include <ggl/utility.hpp>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>

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

    constexpr Buffer(std::span<uint8_t> bytes) noexcept
        : GglBuffer { bytes.data(), bytes.size() } {
    }

    Buffer(std::string_view sv) noexcept
        : Buffer { reinterpret_cast<uint8_t *>(const_cast<char *>(sv.data())),
                   sv.size() } {
    }

    constexpr Buffer(const GglBuffer &buffer) noexcept
        : GglBuffer { buffer } {
    }

    constexpr Buffer(pointer buf, size_type size) noexcept
        : GglBuffer { buf, size } {
    }

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

    constexpr bool operator==(const Buffer &rhs) const noexcept {
        return (size() == rhs.size()) && std::ranges::equal(*this, rhs);
    }

    constexpr bool operator!=(const Buffer &rhs) const noexcept {
        return !(*this == rhs);
    }
};

}

#endif
