// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_EVENTSTREAM_H
#define GGL_EVENTSTREAM_H

//! AWS EventStream message encoding.

#include <ggl/attr.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/eventstream/types.h>
#include <ggl/io.h>
#include <stddef.h>

/// Encode an EventStream packet into a buffer.
/// Payload must fail if it does not fit in provided buffer.
GglError eventstream_encode(
    GglBuffer buf[static 1],
    const EventStreamHeader *headers,
    size_t header_count,
    GglReader payload
) NONNULL_IF_NONZERO(2, 3);

#endif
