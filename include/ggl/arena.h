// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_ARENA_H
#define GGL_ARENA_H

//! Arena allocation

#include <ggl/attr.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/object.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Arena allocator backed by fixed buffer
typedef struct {
    uint8_t *mem;
    uint32_t capacity;
    uint32_t index;
} GglArena;

typedef struct {
    uint32_t index;
} GglArenaState;

/// Obtain an initialized `GglAlloc` backed by `buf`.
static inline GglArena ggl_arena_init(GglBuffer buf) {
    return (GglArena) { .mem = buf.data,
                        .capacity = buf.len <= UINT32_MAX ? (uint32_t) buf.len
                                                          : UINT32_MAX };
}

/// Allocate a `type` from an arena.
#define GGL_ARENA_ALLOC(arena, type) \
    (typeof(type) *) ggl_arena_alloc(arena, sizeof(type), alignof(type))
/// Allocate `n` units of `type` from an arena.
#define GGL_ARENA_ALLOCN(arena, type, n) \
    (typeof(type) *) ggl_arena_alloc(arena, (n) * sizeof(type), alignof(type))

/// Allocate `size` bytes with given alignment from an arena.
void *ggl_arena_alloc(GglArena *arena, size_t size, size_t alignment)
    ALLOC_ALIGN(3) ACCESS(read_write, 1);

/// Resize ptr's allocation (must be the last allocated ptr).
GglError ggl_arena_resize_last(
    GglArena *arena, const void *ptr, size_t old_size, size_t size
) NONNULL(1, 2) ACCESS(read_write, 1) ACCESS(none, 2);

/// Returns true if arena's mem contains ptr.
bool ggl_arena_owns(const GglArena *arena, const void *ptr) PURE
    ACCESS(read_only, 1) ACCESS(none, 2);

/// Allocates remaining space into a buffer.
GglBuffer ggl_arena_alloc_rest(GglArena *arena) ACCESS(read_write, 1);

/// Modifies all of an object's references to point into a given arena
GglError ggl_arena_claim_obj(GglObject *obj, GglArena *arena) NONNULL(1)
    ACCESS(read_write, 1) ACCESS(read_write, 2);

/// Modifies an buffer to point into a given arena
GglError ggl_arena_claim_buf(GglBuffer *buf, GglArena *arena) NONNULL(1)
    ACCESS(read_write, 1) ACCESS(read_write, 2);

/// Modifies only the buffers of an object to point into a given arena
GglError ggl_arena_claim_obj_bufs(GglObject *obj, GglArena *arena) NONNULL(1)
    ACCESS(read_write, 1) ACCESS(read_write, 2);

#endif
