// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_ARENA_HPP
#define GG_ARENA_HPP

#include <gg/types.hpp>
#include <gg/utility.hpp>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace gg {

struct Arena : public GgArena {
public:
    constexpr Arena() noexcept = default;

    constexpr Arena(void *bytes, std::size_t size_bytes) noexcept
        : GgArena { .mem = static_cast<std::uint8_t *>(bytes),
                    .capacity = saturate_cast<std::uint32_t>(size_bytes),
                    .index = 0 } {
    }

    constexpr Arena &operator=(const Arena &) noexcept = default;
    constexpr Arena(const Arena &) noexcept = default;

    constexpr Arena(const GgArena &arena) noexcept
        : GgArena { arena } {
    }

    Arena &operator=(const GgArena &arena) noexcept {
        return *this = Arena { arena };
    }

    constexpr friend void swap(Arena &lhs, Arena &rhs) noexcept {
        lhs = std::exchange(rhs, lhs);
    }

    void *do_allocate(
        std::size_t size, std::size_t alignment = alignof(std::max_align_t)
    ) noexcept {
        return gg_arena_alloc(this, size, alignment);
    }
};

}

static_assert(
    std::is_standard_layout_v<gg::Arena>,
    "gg::Arena must not declare any virtual functions or data members."
);

#endif
