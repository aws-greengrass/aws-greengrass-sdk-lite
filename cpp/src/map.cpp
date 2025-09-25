// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/buffer.hpp>
#include <ggl/map.hpp>
#include <ggl/object.hpp>
#include <ggl/types.hpp>

namespace ggl {

Object *KV::value() noexcept {
    return static_cast<Object *>(ggl_kv_val(this));
}

KV::KV(Buffer key, const Object &value) noexcept
    : GglKV { ggl_kv(key, value) } {
}

}
