// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_BASE64_H
#define GGL_BASE64_H

//! Base64 utilities

#include <ggl/arena.h>
#include <ggl/attr.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <stdbool.h>

/// Convert a base64 buffer to its decoded data.
/// Target must be large enough to hold decoded value.
ACCESS(read_write, 2)
bool ggl_base64_decode(GglBuffer base64, GglBuffer target[static 1]);

/// Convert a base64 buffer to its decoded data in place.
ACCESS(read_write, 1)
bool ggl_base64_decode_in_place(GglBuffer target[static 1]);

/// Encode a buffer into base64.
ACCESS(read_write, 2) ACCESS(write_only, 3)
GglError ggl_base64_encode(
    GglBuffer buf, GglArena *alloc, GglBuffer result[static 1]
);

#endif
