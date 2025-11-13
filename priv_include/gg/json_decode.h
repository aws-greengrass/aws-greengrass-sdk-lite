// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_JSON_DECODE_H
#define GG_JSON_DECODE_H

//! JSON decoding

#include <gg/arena.h>
#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/object.h>

/// Reads a JSON doc from a buffer as a GgObject.
/// Result obj may contain references into buf, and allocations from alloc.
/// Input buffer will be modified.
VISIBILITY(hidden)
GgError gg_json_decode_destructive(GgBuffer buf, GgArena *arena, GgObject *obj);

#endif
