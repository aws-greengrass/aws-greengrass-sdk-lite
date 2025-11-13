// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_EVENTSTREAM_RPC_H
#define GG_EVENTSTREAM_RPC_H

//! AWS EventStream message data types.

#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/eventstream/decode.h>
#include <gg/io.h>
#include <stdint.h>

/// `:message-type` header values
typedef enum {
    EVENTSTREAM_APPLICATION_MESSAGE = 0,
    EVENTSTREAM_APPLICATION_ERROR = 1,
    EVENTSTREAM_CONNECT = 4,
    EVENTSTREAM_CONNECT_ACK = 5,
} EventStreamMessageType;

/// `:message-flags` header flags
typedef enum FLAG_ENUM {
    EVENTSTREAM_CONNECTION_ACCEPTED = 1,
    EVENTSTREAM_TERMINATE_STREAM = 2,
} EventStreamMessageFlags;

#define EVENTSTREAM_FLAGS_MASK ((int32_t) 3)

typedef struct {
    int32_t stream_id;
    int32_t message_type;
    int32_t message_flags;
} EventStreamCommonHeaders;

/// Get an EventStream packet from an input source
VISIBILITY(hidden)
GgError eventsteam_get_packet(
    GgReader input, EventStreamMessage msg[static 1], GgBuffer buffer
);

/// Decode common EventStream headers
VISIBILITY(hidden)
GgError eventstream_get_common_headers(
    EventStreamMessage *msg, EventStreamCommonHeaders *out
);

#endif
