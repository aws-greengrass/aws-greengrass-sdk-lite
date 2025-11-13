// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_MOCK_TYPES_HPP
#define GG_MOCK_TYPES_HPP

//! AWS EventStream message data types.

extern "C" {
#include <gg/attr.h>
}
#include <cstdint>
#include <gg/types.hpp>

extern "C" {
/// `:message-type` header values
typedef enum : int8_t {
    EVENTSTREAM_APPLICATION_MESSAGE = 0,
    EVENTSTREAM_APPLICATION_ERROR = 1,
    EVENTSTREAM_CONNECT = 4,
    EVENTSTREAM_CONNECT_ACK = 5,
} EventStreamMessageType;

/// `:message-flags` header flags
typedef enum FLAG_ENUM : int8_t {
    EVENTSTREAM_CONNECTION_ACCEPTED = 1,
    EVENTSTREAM_TERMINATE_STREAM = 2,
} EventStreamMessageFlags;

constexpr int32_t EVENTSTREAM_FLAGS_MASK { 3 };

typedef struct {
    int32_t stream_id;
    int32_t message_type;
    int32_t message_flags;
} EventStreamCommonHeaders;

/// Type of EventStream header value.
/// Contains only subset of types used by GG IPC.
typedef enum : int8_t {
    EVENTSTREAM_INT32 = 4,
    EVENTSTREAM_STRING = 7,
} EventStreamHeaderValueType;

/// An EventStream header value.
typedef struct {
    EventStreamHeaderValueType type;

    union {
        int32_t int32;
        GgBuffer string;
    };
} EventStreamHeaderValue;

/// An EventStream header.
typedef struct {
    GgBuffer name;
    EventStreamHeaderValue value;
} EventStreamHeader;
}

#endif
