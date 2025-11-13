// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_MAP_H
#define GG_MAP_H

//! Map utilities

#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/flags.h>
#include <gg/object.h>
#include <stdbool.h>
#include <stddef.h>

// NOLINTBEGIN(bugprone-macro-parentheses)
/// Loop over the KV pairs in a map.
#define GG_MAP_FOREACH(name, map) \
    for (GgKV *name = (map).pairs; name < &(map).pairs[(map).len]; \
         name = &name[1])

// NOLINTEND(bugprone-macro-parentheses)

/// Get the value corresponding with a key.
/// Returns whether the key was found in the map.
/// If `result` is not NULL it is set to the found value or NULL.
ACCESS(write_only, 3)
bool gg_map_get(GgMap map, GgBuffer key, GgObject **result);

/// Get the value from a nested map corresponding with a key path.
/// Returns whether the key was found in the map.
/// If `result` is not NULL it is set to the found value or NULL.
ACCESS(write_only, 3)
bool gg_map_get_path(GgMap map, GgBufList path, GgObject **result);

/// Construct a GgKV.
CONST
GgKV gg_kv(GgBuffer key, GgObject val);

/// Get a GgKV's key.
CONST
GgBuffer gg_kv_key(GgKV kv);

/// Set a GgKV's key.
ACCESS(write_only, 1)
void gg_kv_set_key(GgKV *kv, GgBuffer key);

/// Get a GgKV's value.
CONST ACCESS(none, 1)
GgObject *gg_kv_val(GgKV *kv);

/// Entry in a map validation schema.
typedef struct {
    GgBuffer key;
    GgPresence required;
    GgObjectType type;
    GgObject **value;
} GgMapSchemaEntry;

/// Schema for validating map structure and types.
typedef struct {
    const GgMapSchemaEntry *entries;
    size_t entry_count;
} GgMapSchema;

#define GG_MAP_SCHEMA(...) \
    (GgMapSchema) { \
        .entries = (const GgMapSchemaEntry[]) { __VA_ARGS__ }, \
        .entry_count = (sizeof((GgMapSchemaEntry[]) { __VA_ARGS__ })) \
            / (sizeof(GgMapSchemaEntry)) \
    }

/// Validate a map against a schema.
/// Checks for required keys, validates types, and extracts values.
/// Sets `entry->value` pointers for found keys (or NULL if not found).
/// Returns GG_ERR_OK on success, GG_ERR_NOENTRY if required key missing,
/// or GG_ERR_PARSE if type mismatch or MISSING key is present.
GgError gg_map_validate(GgMap map, GgMapSchema schema);

#endif
