// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_BASE64_H
#define GG_BASE64_H

//! Base64 utilities

#include <gg/arena.h>
#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <stdbool.h>

/// Decode a base64-encoded buffer into raw bytes.
/// `base64` input must be padded (length divisible by 4).
/// `target` must have capacity of at least (base64.len / 4) * 3 bytes.
/// On success, `target->len` is updated to the actual decoded length.
/// Returns false if input is invalid base64 or target is too small.
ACCESS(read_write, 2)
bool gg_base64_decode(GgBuffer base64, GgBuffer target[static 1]);

/// Decode a base64-encoded buffer into raw bytes in place.
/// `target` input must be padded (length divisible by 4).
/// On success, `target->len` is updated to the actual decoded length.
/// Returns false if input is invalid base64.
ACCESS(read_write, 1)
bool gg_base64_decode_in_place(GgBuffer target[static 1]);

/// Encode a buffer into padded base64.
/// Allocates ((buf.len + 2) / 3) * 4 bytes from `alloc` for the result.
/// On success, `result` is set to the encoded base64 buffer.
/// Returns GG_ERR_NOMEM if allocation fails, GG_ERR_OK on success.
ACCESS(read_write, 2) ACCESS(write_only, 3)
GgError gg_base64_encode(
    GgBuffer buf, GgArena *alloc, GgBuffer result[static 1]
);

#endif
