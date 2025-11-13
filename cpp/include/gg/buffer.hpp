// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_BUFFER_HPP
#define GG_BUFFER_HPP

#include <gg/types.hpp>
#include <gg/utility.hpp>
#include <cstdint>
#include <algorithm>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace gg {

class Buffer : public GgBuffer {
public:
    using element_type = std::remove_pointer_t<decltype(GgBuffer::data)>;
    using reference = element_type &;
    using pointer = element_type *;
    using iterator = pointer;
    using size_type = decltype(GgBuffer::len);

    constexpr Buffer() noexcept = default;
    constexpr Buffer(const Buffer &) noexcept = default;

    constexpr Buffer(std::span<uint8_t> bytes) noexcept
        : GgBuffer { bytes.data(), bytes.size() } {
    }

    Buffer(std::string_view sv) noexcept
        : Buffer { reinterpret_cast<uint8_t *>(const_cast<char *>(sv.data())),
                   sv.size() } {
    }

    constexpr Buffer(const GgBuffer &buffer) noexcept
        : GgBuffer { buffer } {
    }

    constexpr Buffer(pointer buf, size_type size) noexcept
        : GgBuffer { buf, size } {
    }

    constexpr Buffer &operator=(const Buffer &) noexcept = default;

    constexpr Buffer &operator=(const GgBuffer &buffer) noexcept {
        return *this = Buffer { buffer };
    }

    constexpr pointer data() const noexcept {
        return GgBuffer::data;
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
            GG_THROW_OR_ABORT(std::out_of_range { "Buffer:at" });
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
