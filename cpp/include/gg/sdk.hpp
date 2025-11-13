// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_SDK_HPP
#define GG_SDK_HPP

namespace gg {
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
    void gg_sdk_init(void);
    gg_sdk_init();
}
}

}

#endif
