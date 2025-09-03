#ifndef GGL_ARENA_HPP
#define GGL_ARENA_HPP

#include "ggl/types.hpp"
#include "ggl/utility.hpp"
#include <cstddef>
#include <cstdint>
#include <utility>

namespace ggl {

struct Arena : public GglArena {
public:
    constexpr Arena() noexcept = default;

    constexpr Arena(void *bytes, std::size_t size_bytes) noexcept
        : GglArena { .mem = static_cast<std::uint8_t *>(bytes),
                     .capacity = saturate_cast<std::uint32_t>(size_bytes),
                     .index = 0 } {
    }

    constexpr Arena &operator=(const Arena &) noexcept = default;
    constexpr Arena(const Arena &) noexcept = default;

    constexpr Arena(const GglArena &arena) noexcept
        : GglArena { arena } {
    }

    Arena &operator=(const GglArena &arena) noexcept {
        return *this = Arena { arena };
    }

    constexpr friend void swap(Arena &lhs, Arena &rhs) noexcept {
        lhs = std::exchange(rhs, lhs);
    }

    void *do_allocate(
        std::size_t size, std::size_t alignment = alignof(std::max_align_t)
    ) noexcept {
        return ggl_arena_alloc(this, size, alignment);
    }
};

}

static_assert(
    std::is_standard_layout_v<ggl::Arena>,
    "ggl::Arena must not declare any virtual functions or data members."
);

#endif
