// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_SDK_HPP
#define GGL_SDK_HPP

namespace ggl {
class Sdk {
private:
    Sdk() noexcept;

public:
    static Sdk &get() noexcept {
        static Sdk singleton {};
        return singleton;
    }
};

extern "C" {
inline Sdk::Sdk() noexcept {
    void ggl_sdk_init(void);
    ggl_sdk_init();
}
}

}

#endif
