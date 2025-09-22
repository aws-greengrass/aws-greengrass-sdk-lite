#ifndef GGL_LIST_HPP
#define GGL_LIST_HPP

#include <ggl/error.hpp>
#include <ggl/types.hpp>
#include <ggl/utility.hpp>
#include <cstddef>
#include <stdexcept>
#include <type_traits>

namespace ggl {

class Object;

class List : public GglList {
public:
    using element_type = Object;
    using value_type = std::remove_cv_t<element_type>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = element_type *;
    using const_pointer = const element_type *;
    using reference = element_type &;
    using iterator = pointer;
    using const_iterator = const_pointer;

    constexpr List() noexcept = default;

    constexpr List(GglObject *data, size_t size) noexcept
        : GglList { data, size } {
    }

    constexpr List(GglList list) noexcept
        : GglList { list } {
    }

    constexpr List(const List &rhs) noexcept = default;
    constexpr List &operator=(const List &rhs) noexcept = default;

    // observers

    constexpr size_type size() const noexcept {
        return len;
    }

    constexpr bool empty() const noexcept {
        return size() == 0U;
    }

    // return GGL_ERR_OK if all elements are of the specified type
    GglError type_check(GglObjectType type) const noexcept {
        return ggl_list_type_check(*this, type);
    }

    // element access

    // UB if empty
    reference front() const noexcept {
        return operator[](0);
    }

    // UB if empty
    reference back() const noexcept {
        return operator[](size() - 1);
    }

    reference at(size_type pos) const {
        if (pos >= size()) {
            GGL_THROW_OR_ABORT(
                std::out_of_range("ggl::List::at: out of range")
            );
        }

        return operator[](pos);
    }

    // UB if out of range
    reference operator[](size_type pos) const noexcept;

    pointer data() const noexcept;

    // iterators

    iterator begin() const noexcept {
        return data();
    }

    iterator end() const noexcept {
        return &operator[](size());
    }

    const_iterator cbegin() const noexcept {
        return data();
    }

    const_iterator cend() const noexcept {
        return &operator[](size());
    }
};
}

static_assert(
    std::is_standard_layout_v<ggl::List>,
    "ggl::List must not declare any virtual functions or data members."
);

#endif
