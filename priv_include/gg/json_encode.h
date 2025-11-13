// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_JSON_ENCODE_H
#define GG_JSON_ENCODE_H

//! JSON encoding

#include <gg/attr.h>
#include <gg/error.h>
#include <gg/io.h>
#include <gg/object.h>

/// Serializes a GgObject into a buffer in JSON encoding.
VISIBILITY(hidden)
GgError gg_json_encode(GgObject obj, GgWriter writer);

/// Reader from which a JSON-serialized object can be read.
/// Errors if buffer is not large enough for entire object.
VISIBILITY(hidden)
GgReader gg_json_reader(const GgObject *obj);

#endif
