// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_OBJECT_H
#define GG_OBJECT_H

//! Generic dynamic object representation.

#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Maximum depth of an object.
/// i.e. `5` has depth 1, `{"a":5}` is depth 2, and `[{"a":5}]` is 3.
#define GG_MAX_OBJECT_DEPTH (15U)

/// Maximum subobject count for an object.
/// Subobject count calculation:
///   subobject_count(non-list/map object) = 0
///   subobject_count(list) = len + sum({item: subobject_count(item)})
///   subobject_count(map) = 2 * len + sum({pair: subobject_count(pair.value))})
#define GG_MAX_OBJECT_SUBOBJECTS (255U)

/// A generic object.
typedef struct {
    // Used only with memcpy so no aliasing with contents
    uint8_t _private[(sizeof(void *) == 4) ? 9 : 11];
} GgObject;

/// Type tag for `GgObject`.
typedef enum {
    GG_TYPE_NULL = 0,
    GG_TYPE_BOOLEAN,
    GG_TYPE_I64,
    GG_TYPE_F64,
    GG_TYPE_BUF,
    GG_TYPE_LIST,
    GG_TYPE_MAP,
} GgObjectType;

/// An array of `GgObject`.
typedef struct {
    GgObject *items;
    size_t len;
} GgList;

/// A key-value pair used for `GgMap`.
/// `key` must be an UTF-8 encoded string.
typedef struct {
    // KVs alias with pointers to their value objects
    uint8_t _private[sizeof(void *) + 2 + sizeof(GgObject)];
} GgKV;

/// A map of UTF-8 strings to `GgObject`s.
typedef struct {
    GgKV *pairs;
    size_t len;
} GgMap;

/// Create list literal from object literals.
#define GG_LIST(...) \
    (GgList) { \
        .items = (GgObject[]) { __VA_ARGS__ }, \
        .len = (sizeof((GgObject[]) { __VA_ARGS__ })) / (sizeof(GgObject)) \
    }

/// Create map literal from key-value literals.
#define GG_MAP(...) \
    (GgMap) { \
        .pairs = (GgKV[]) { __VA_ARGS__ }, \
        .len = (sizeof((GgKV[]) { __VA_ARGS__ })) / (sizeof(GgKV)) \
    }

/// Get type of an GgObject.
CONST
GgObjectType gg_obj_type(GgObject obj);

#define GG_OBJ_NULL (GgObject) { 0 }

/// Create bool object.
CONST
GgObject gg_obj_bool(bool value);

/// Get the bool represented by an object.
/// The GgObject must be of type GG_TYPE_BOOLEAN.
CONST
bool gg_obj_into_bool(GgObject boolean);

/// Create signed integer object.
CONST
GgObject gg_obj_i64(int64_t value);

/// Get the i64 represented by an object.
/// The GgObject must be of type GG_TYPE_I64.
CONST
int64_t gg_obj_into_i64(GgObject i64);

/// Create floating point object.
CONST
GgObject gg_obj_f64(double value);

/// Get the f64 represented by an object.
/// The GgObject must be of type GG_TYPE_F64.
CONST
double gg_obj_into_f64(GgObject f64);

/// Create buffer object.
CONST
GgObject gg_obj_buf(GgBuffer value);

/// Get the buffer represented by an object.
/// The GgObject must be of type GG_TYPE_BUF.
CONST
GgBuffer gg_obj_into_buf(GgObject buf);

/// Create map object.
CONST
GgObject gg_obj_map(GgMap value);

/// Get the map represented by an object.
/// The GgObject must be of type GG_TYPE_MAP.
CONST
GgMap gg_obj_into_map(GgObject map);

/// Create list object.
CONST
GgObject gg_obj_list(GgList value);

/// Get the list represented by an object.
/// The GgObject must be of type GG_TYPE_LIST.
CONST
GgList gg_obj_into_list(GgObject list);

/// Calculate max memory needed to claim an object.
/// This is the max memory used by gg_arena_claim_obj on this object.
/// On success, sets `size` to the calculated memory requirement.
/// Returns GG_ERR_OK on success.
ACCESS(write_only, 2) REPRODUCIBLE
GgError gg_obj_mem_usage(GgObject obj, size_t *size);

#endif
