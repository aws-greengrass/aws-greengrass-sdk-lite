// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_LIST_HPP
#define GG_LIST_HPP

#include <gg/error.hpp>
#include <gg/types.hpp>
#include <gg/utility.hpp>
#include <cstddef>
#include <type_traits>

namespace gg {

class Object;

class List : public GgList {
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

    constexpr List(GgObject *data, size_t size) noexcept
        : GgList { data, size } {
    }

    constexpr List(GgList list) noexcept
        : GgList { list } {
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

    // return GG_ERR_OK if all elements are of the specified type
    GgError type_check(GgObjectType type) const noexcept {
        return gg_list_type_check(*this, type);
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
            GG_THROW_OR_ABORT(
                gg::Exception(GG_ERR_RANGE, "gg::List::at: out of range")
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
    std::is_standard_layout_v<gg::List>,
    "gg::List must not declare any virtual functions or data members."
);

#endif
