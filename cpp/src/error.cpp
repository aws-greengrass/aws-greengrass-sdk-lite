// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/error.hpp>
#include <system_error>

extern "C" {
[[gnu::const]]
const char *ggl_strerror(GglError err) noexcept;
}

namespace ggl::detail {
class category : public std::error_category {
public:
    const char *name() const noexcept override {
        return "ggl::category";
    }

    std::string message(int value) const override {
        return ::ggl_strerror(static_cast<GglError>(value));
    }
};
}

namespace ggl {
const std::error_category &category() noexcept {
    static detail::category instance;
    return instance;
}

}
