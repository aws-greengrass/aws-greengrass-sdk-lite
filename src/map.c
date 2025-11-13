// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/flags.h>
#include <gg/log.h>
#include <gg/map.h>
#include <gg/object.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// This assumption is used to size buffers in a lot of places
static_assert(
    sizeof(GgKV) <= 2 * sizeof(GgObject),
    "GgKV must be at most the size of two GgObjects."
);

COLD
static void length_err(size_t *len) {
    GG_LOGE(
        "Key length longer than can be stored in GgKV (%zu, max %u).",
        *len,
        (unsigned int) UINT16_MAX
    );
    assert(false);
    *len = UINT16_MAX;
}

GgKV gg_kv(GgBuffer key, GgObject val) {
    GgKV result = { 0 };

    static_assert(
        sizeof(result._private) >= sizeof(void *) + 2 + sizeof(GgObject),
        "GgKV must be able to hold key pointer, 16-bit key length, and value."
    );

    gg_kv_set_key(&result, key);

    memcpy(&result._private[sizeof(void *) + 2], &val, sizeof(GgObject));
    return result;
}

GgBuffer gg_kv_key(GgKV kv) {
    void *ptr;
    uint16_t len;
    memcpy(&ptr, kv._private, sizeof(void *));
    memcpy(&len, &kv._private[sizeof(void *)], 2);
    return (GgBuffer) { .data = ptr, .len = len };
}

void gg_kv_set_key(GgKV *kv, GgBuffer key) {
    if (key.len > UINT16_MAX) {
        length_err(&key.len);
    }
    uint16_t key_len = (uint16_t) key.len;
    memcpy(kv->_private, &key.data, sizeof(void *));
    memcpy(&kv->_private[sizeof(void *)], &key_len, 2);
}

GgObject *gg_kv_val(GgKV *kv) {
    return (GgObject *) &kv->_private[sizeof(void *) + 2];
}

bool gg_map_get(GgMap map, GgBuffer key, GgObject **result) {
    GG_MAP_FOREACH(pair, map) {
        if (gg_buffer_eq(key, gg_kv_key(*pair))) {
            if (result != NULL) {
                *result = gg_kv_val(pair);
            }
            return true;
        }
    }
    if (result != NULL) {
        *result = NULL;
    }
    return false;
}

bool gg_map_get_path(GgMap map, GgBufList path, GgObject **result) {
    assert(path.len >= 1);

    GgMap current = map;
    for (size_t i = 0; i < path.len - 1; i++) {
        GgObject *item;
        if (!gg_map_get(current, path.bufs[i], &item)) {
            return false;
        }
        if (gg_obj_type(*item) != GG_TYPE_MAP) {
            return false;
        }
        current = gg_obj_into_map(*item);
    }

    return gg_map_get(current, path.bufs[path.len - 1], result);
}

GgError gg_map_validate(GgMap map, GgMapSchema schema) {
    for (size_t i = 0; i < schema.entry_count; i++) {
        const GgMapSchemaEntry *entry = &schema.entries[i];
        GgObject *value;
        bool found = gg_map_get(map, entry->key, &value);
        if (!found) {
            if (entry->required.val == GG_PRESENCE_REQUIRED) {
                GG_LOGE(
                    "Map missing required key %.*s.",
                    (int) entry->key.len,
                    entry->key.data
                );
                return GG_ERR_NOENTRY;
            }

            if (entry->required.val == GG_PRESENCE_OPTIONAL) {
                GG_LOGT(
                    "Missing optional key %.*s.",
                    (int) entry->key.len,
                    entry->key.data
                );
            }

            if (entry->value != NULL) {
                *entry->value = NULL;
            }
            continue;
        }

        GG_LOGT(
            "Found key %.*s with len %zu",
            (int) entry->key.len,
            entry->key.data,
            entry->key.len
        );

        if (entry->required.val == GG_PRESENCE_MISSING) {
            GG_LOGE(
                "Map has required missing key %.*s.",
                (int) entry->key.len,
                entry->key.data
            );
            return GG_ERR_PARSE;
        }

        if (entry->type != GG_TYPE_NULL) {
            if (entry->type != gg_obj_type(*value)) {
                GG_LOGE(
                    "Key %.*s is of invalid type.",
                    (int) entry->key.len,
                    entry->key.data
                );
                return GG_ERR_PARSE;
            }
        }

        if (entry->value != NULL) {
            *entry->value = value;
        }
    }

    return GG_ERR_OK;
}
