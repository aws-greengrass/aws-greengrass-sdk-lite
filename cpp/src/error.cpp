// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/error.hpp>
#include <system_error>

extern "C" {
[[gnu::const]]
const char *gg_strerror(GgError err) noexcept;
}

namespace gg::detail {
class category : public std::error_category {
public:
    const char *name() const noexcept override {
        return "gg::category";
    }

    std::string message(int value) const override {
        return ::gg_strerror(static_cast<GgError>(value));
    }
};
}

namespace gg {
const std::error_category &category() noexcept {
    static detail::category instance;
    return instance;
}

}
