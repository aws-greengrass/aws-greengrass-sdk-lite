// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_TYPES_HPP
#define GG_TYPES_HPP

#include <gg/error.hpp>
#include <cstddef>
#include <cstdint>

extern "C" {
typedef struct {
    uint8_t _private[(sizeof(void *) == 4) ? 9 : 11];
} GgObject;

typedef struct {
    uint8_t *data;
    size_t len;
} GgBuffer;

typedef struct {
    GgBuffer *bufs;
    size_t len;
} GgBufList;

typedef struct {
    GgObject *items;
    size_t len;
} GgList;

typedef struct {
    uint8_t _private[sizeof(void *) + 2 + sizeof(GgObject)];
} GgKV;

typedef struct {
    GgKV *pairs;
    size_t len;
} GgMap;

typedef struct {
    void *mem;
    uint32_t capacity;
    uint32_t index;
} GgArena;

/// Type tag for `GgObject`.
// NOLINTNEXTLINE(performance-enum-size)
typedef enum {
    GG_TYPE_NULL = 0,
    GG_TYPE_BOOLEAN,
    GG_TYPE_I64,
    GG_TYPE_F64,
    GG_TYPE_BUF,
    GG_TYPE_LIST,
    GG_TYPE_MAP,
} GgObjectType;

typedef struct {
    uint32_t val;
} GgIpcSubscriptionHandle;

// NOLINTNEXTLINE(performance-enum-size)
enum class GgComponentState {
    RUNNING,
    ERRORED
};

namespace gg {
    using ComponentState = GgComponentState;
}

[[gnu::const]]
GgObjectType gg_obj_type(GgObject) noexcept;
[[gnu::const]]
bool gg_obj_into_bool(GgObject) noexcept;
[[gnu::const]]
int64_t gg_obj_into_i64(GgObject) noexcept;
[[gnu::const]]
double gg_obj_into_f64(GgObject) noexcept;
[[gnu::const]]
GgBuffer gg_obj_into_buf(GgObject) noexcept;
[[gnu::const]]
GgList gg_obj_into_list(GgObject) noexcept;
[[gnu::const]]
GgMap gg_obj_into_map(GgObject) noexcept;

[[gnu::const]]
GgObject gg_obj_bool(bool) noexcept;
[[gnu::const]]
GgObject gg_obj_i64(int64_t) noexcept;
[[gnu::const]]
GgObject gg_obj_f64(double) noexcept;
[[gnu::const]]
GgObject gg_obj_buf(GgBuffer) noexcept;
[[gnu::const]]
GgObject gg_obj_list(GgList) noexcept;
[[gnu::const]]
GgObject gg_obj_map(GgMap) noexcept;

[[gnu::const]]
GgKV gg_kv(GgBuffer, GgObject) noexcept;
[[gnu::const]]
GgObject *gg_kv_val(GgKV *) noexcept;
[[gnu::const]]
GgBuffer gg_kv_key(GgKV) noexcept;
void gg_kv_set_key(GgKV *kv, GgBuffer key) noexcept;

[[gnu::pure]]
GgError gg_list_type_check(GgList list, GgObjectType type) noexcept;

void *gg_arena_alloc(GgArena *arena, size_t size, size_t alignment) noexcept;

GgError gg_obj_mem_usage(GgObject obj, size_t *size) noexcept;
}

#endif
