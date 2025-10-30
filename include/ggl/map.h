// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_MAP_H
#define GGL_MAP_H

//! Map utilities

#include <ggl/attr.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/flags.h>
#include <ggl/object.h>
#include <stdbool.h>
#include <stddef.h>

// NOLINTBEGIN(bugprone-macro-parentheses)
/// Loop over the KV pairs in a map.
#define GGL_MAP_FOREACH(name, map) \
    for (GglKV *name = (map).pairs; name < &(map).pairs[(map).len]; \
         name = &name[1])

// NOLINTEND(bugprone-macro-parentheses)

/// Get the value corresponding with a key.
/// Returns whether the key was found in the map.
/// If `result` is not NULL it is set to the found value or NULL.
ACCESS(write_only, 3)
bool ggl_map_get(GglMap map, GglBuffer key, GglObject **result);

/// Get the value from a nested map corresponding with a key path.
/// Returns whether the key was found in the map.
/// If `result` is not NULL it is set to the found value or NULL.
ACCESS(write_only, 3)
bool ggl_map_get_path(GglMap map, GglBufList path, GglObject **result);

/// Construct a GglKV.
CONST
GglKV ggl_kv(GglBuffer key, GglObject val);

/// Get a GglKV's key.
CONST
GglBuffer ggl_kv_key(GglKV kv);

/// Set a GglKV's key.
ACCESS(write_only, 1)
void ggl_kv_set_key(GglKV *kv, GglBuffer key);

/// Get a GglKV's value.
CONST ACCESS(none, 1)
GglObject *ggl_kv_val(GglKV *kv);

/// Entry in a map validation schema.
typedef struct {
    GglBuffer key;
    GglPresence required;
    GglObjectType type;
    GglObject **value;
} GglMapSchemaEntry;

/// Schema for validating map structure and types.
typedef struct {
    const GglMapSchemaEntry *entries;
    size_t entry_count;
} GglMapSchema;

#define GGL_MAP_SCHEMA(...) \
    (GglMapSchema) { \
        .entries = (const GglMapSchemaEntry[]) { __VA_ARGS__ }, \
        .entry_count = (sizeof((GglMapSchemaEntry[]) { __VA_ARGS__ })) \
            / (sizeof(GglMapSchemaEntry)) \
    }

/// Validate a map against a schema.
/// Checks for required keys, validates types, and extracts values.
/// Sets `entry->value` pointers for found keys (or NULL if not found).
/// Returns GGL_ERR_OK on success, GGL_ERR_NOENTRY if required key missing,
/// or GGL_ERR_PARSE if type mismatch or MISSING key is present.
GglError ggl_map_validate(GglMap map, GglMapSchema schema);

#endif
