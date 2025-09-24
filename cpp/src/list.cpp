// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/list.hpp>
#include <ggl/object.hpp>

namespace ggl {

List::reference List::operator[](size_type pos) const noexcept {
    return data()[pos];
}

List::pointer List::data() const noexcept {
    return static_cast<Object *>(items);
}

}
