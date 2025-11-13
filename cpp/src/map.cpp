// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/buffer.hpp>
#include <gg/map.hpp>
#include <gg/object.hpp>
#include <gg/types.hpp>

namespace gg {

Object *KV::value() noexcept {
    return static_cast<Object *>(gg_kv_val(this));
}

KV::KV(Buffer key, const Object &value) noexcept
    : GgKV { gg_kv(key, value) } {
}

}
