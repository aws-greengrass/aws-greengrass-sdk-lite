// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_OBJECT_H
#define GGL_OBJECT_H

//! Generic dynamic object representation.

#include <ggl/attr.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Maximum depth of an object.
/// i.e. `5` has depth 1, `{"a":5}` is depth 2, and `[{"a":5}]` is 3.
#define GGL_MAX_OBJECT_DEPTH (15U)

/// Maximum subobject count for an object.
/// Subobject count calculation:
///   subobject_count(non-list/map object) = 0
///   subobject_count(list) = len + sum({item: subobject_count(item)})
///   subobject_count(map) = 2 * len + sum({pair: subobject_count(pair.value))})
#define GGL_MAX_OBJECT_SUBOBJECTS (255U)

/// A generic object.
typedef struct {
    // Used only with memcpy so no aliasing with contents
    uint8_t _private[(sizeof(void *) == 4) ? 9 : 11];
} GglObject;

/// Type tag for `GglObject`.
typedef enum {
    GGL_TYPE_NULL = 0,
    GGL_TYPE_BOOLEAN,
    GGL_TYPE_I64,
    GGL_TYPE_F64,
    GGL_TYPE_BUF,
    GGL_TYPE_LIST,
    GGL_TYPE_MAP,
} GglObjectType;

/// An array of `GglObject`.
typedef struct {
    GglObject *items;
    size_t len;
} GglList;

/// A key-value pair used for `GglMap`.
/// `key` must be an UTF-8 encoded string.
typedef struct {
    // KVs alias with pointers to their value objects
    uint8_t _private[sizeof(void *) + 2 + sizeof(GglObject)];
} GglKV;

/// A map of UTF-8 strings to `GglObject`s.
typedef struct {
    GglKV *pairs;
    size_t len;
} GglMap;

/// Create list literal from object literals.
#define GGL_LIST(...) \
    (GglList) { \
        .items = (GglObject[]) { __VA_ARGS__ }, \
        .len = (sizeof((GglObject[]) { __VA_ARGS__ })) / (sizeof(GglObject)) \
    }

/// Create map literal from key-value literals.
#define GGL_MAP(...) \
    (GglMap) { \
        .pairs = (GglKV[]) { __VA_ARGS__ }, \
        .len = (sizeof((GglKV[]) { __VA_ARGS__ })) / (sizeof(GglKV)) \
    }

/// Get type of an GglObject.
CONST
GglObjectType ggl_obj_type(GglObject obj);

#define GGL_OBJ_NULL (GglObject) { 0 }

/// Create bool object.
CONST
GglObject ggl_obj_bool(bool value);

/// Get the bool represented by an object.
/// The GglObject must be of type GGL_TYPE_BOOLEAN.
CONST
bool ggl_obj_into_bool(GglObject boolean);

/// Create signed integer object.
CONST
GglObject ggl_obj_i64(int64_t value);

/// Get the i64 represented by an object.
/// The GglObject must be of type GGL_TYPE_I64.
CONST
int64_t ggl_obj_into_i64(GglObject i64);

/// Create floating point object.
CONST
GglObject ggl_obj_f64(double value);

/// Get the f64 represented by an object.
/// The GglObject must be of type GGL_TYPE_F64.
CONST
double ggl_obj_into_f64(GglObject f64);

/// Create buffer object.
CONST
GglObject ggl_obj_buf(GglBuffer value);

/// Get the buffer represented by an object.
/// The GglObject must be of type GGL_TYPE_BUF.
CONST
GglBuffer ggl_obj_into_buf(GglObject buf);

/// Create map object.
CONST
GglObject ggl_obj_map(GglMap value);

/// Get the map represented by an object.
/// The GglObject must be of type GGL_TYPE_MAP.
CONST
GglMap ggl_obj_into_map(GglObject map);

/// Create list object.
CONST
GglObject ggl_obj_list(GglList value);

/// Get the list represented by an object.
/// The GglObject must be of type GGL_TYPE_LIST.
CONST
GglList ggl_obj_into_list(GglObject list);

/// Calculate max memory needed to claim an object.
/// This is the max memory used by ggl_arena_claim_obj on this object.
/// On success, sets `size` to the calculated memory requirement.
/// Returns GGL_ERR_OK on success.
ACCESS(write_only, 2) REPRODUCIBLE
GglError ggl_obj_mem_usage(GglObject obj, size_t *size);

#endif
