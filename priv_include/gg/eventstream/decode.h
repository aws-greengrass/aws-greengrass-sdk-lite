// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_EVENTSTREAM_DECODE_H
#define GG_EVENTSTREAM_DECODE_H

//! AWS EventStream message decoding.

#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/eventstream/types.h>
#include <stdint.h>

/// An iterator over EventStream headers.
typedef struct {
    uint32_t count;
    uint8_t *pos;
} EventStreamHeaderIter;

/// A parsed EventStream packet.
typedef struct {
    uint32_t data_len;
    uint32_t headers_len;
    uint32_t crc;
} EventStreamPrelude;

/// A parsed EventStream packet.
typedef struct {
    EventStreamHeaderIter headers;
    GgBuffer payload;
} EventStreamMessage;

/// Parse an EventStream packet prelude from a buffer.
VISIBILITY(hidden)
GgError eventstream_decode_prelude(GgBuffer buf, EventStreamPrelude *prelude);

/// Parse an EventStream packet data section from a buffer.
/// The buffer should contain the rest of the packet after the prelude.
/// Parsed struct representation holds references into the buffer.
/// Validates headers.
VISIBILITY(hidden)
GgError eventstream_decode(
    const EventStreamPrelude *prelude,
    GgBuffer data_section,
    EventStreamMessage *msg
);

/// Get the next header from an EventStreamHeaderIter.
/// Mutates the iter to refer to the rest of the headers.
/// Assumes headers already validated by decode.
VISIBILITY(hidden)
GgError eventstream_header_next(
    EventStreamHeaderIter *headers, EventStreamHeader *header
);

#endif
