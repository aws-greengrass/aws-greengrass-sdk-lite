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

/// Decode a base64-encoded buffer into raw bytes.
/// `base64` input must be padded (length divisible by 4).
/// `target` must have capacity of at least (base64.len / 4) * 3 bytes.
/// On success, `target->len` is updated to the actual decoded length.
/// Returns false if input is invalid base64 or target is too small.
ACCESS(read_write, 2)
bool ggl_base64_decode(GglBuffer base64, GglBuffer target[static 1]);

/// Decode a base64-encoded buffer into raw bytes in place.
/// `target` input must be padded (length divisible by 4).
/// On success, `target->len` is updated to the actual decoded length.
/// Returns false if input is invalid base64.
ACCESS(read_write, 1)
bool ggl_base64_decode_in_place(GglBuffer target[static 1]);

/// Encode a buffer into padded base64.
/// Allocates ((buf.len + 2) / 3) * 4 bytes from `alloc` for the result.
/// On success, `result` is set to the encoded base64 buffer.
/// Returns GGL_ERR_NOMEM if allocation fails, GGL_ERR_OK on success.
ACCESS(read_write, 2) ACCESS(write_only, 3)
GglError ggl_base64_encode(
    GglBuffer buf, GglArena *alloc, GglBuffer result[static 1]
);

#endif
