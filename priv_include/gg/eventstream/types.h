// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_EVENTSTREAM_TYPES_H
#define GG_EVENTSTREAM_TYPES_H

//! AWS EventStream message data types.

#include <gg/buffer.h>
#include <stdint.h>

/// Type of EventStream header value.
/// Contains only subset of types used by GG IPC.
typedef enum {
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

#endif
