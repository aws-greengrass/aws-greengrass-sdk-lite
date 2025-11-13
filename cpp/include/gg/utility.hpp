// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_UTILITY_HPP
#define GG_UTILITY_HPP

#include <algorithm>
#include <limits>
#include <type_traits>

#if __cpp_exceptions
#define GG_THROW_OR_ABORT(...) throw(__VA_ARGS__)
#else
#include <cstdlib>
#define GG_THROW_OR_ABORT(...) std::abort()
#endif

namespace gg {
/// Performs a narrowing cast on value, clamping it to the target type's range.
template <class NarrowerT, class WiderT>
constexpr NarrowerT saturate_cast(const WiderT &value) noexcept {
    if constexpr (std::is_signed_v<WiderT>) {
        return static_cast<NarrowerT>(std::clamp<WiderT>(
            value,
            std::numeric_limits<NarrowerT>::lowest(),
            std::numeric_limits<NarrowerT>::max()
        ));
    } else if constexpr (std::numeric_limits<WiderT>::max()
                         > std::numeric_limits<NarrowerT>::max()) {
        return static_cast<NarrowerT>(
            std::min<WiderT>(value, std::numeric_limits<NarrowerT>::max())
        );
    } else {
        return static_cast<NarrowerT>(value);
    }
}
}

#endif
