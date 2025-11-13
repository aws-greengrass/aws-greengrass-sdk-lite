// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_EVENTSTREAM_H
#define GG_EVENTSTREAM_H

//! AWS EventStream message encoding.

#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/eventstream/types.h>
#include <gg/io.h>
#include <stddef.h>

/// Encode an EventStream packet into a buffer.
/// Payload must fail if it does not fit in provided buffer.
VISIBILITY(hidden) NONNULL_IF_NONZERO(2, 3)
GgError eventstream_encode(
    GgBuffer buf[static 1],
    const EventStreamHeader *headers,
    size_t header_count,
    GgReader payload
);

#endif
