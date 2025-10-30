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

/// Arena allocator backed by fixed buffer.
typedef struct {
    uint8_t *mem;
    uint32_t capacity;
    uint32_t index;
} GglArena;

/// Saved state of an arena allocator.
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
/// Returns pointer to allocated memory, or NULL if insufficient space.
/// Alignment must be a power of 2.
ACCESS(read_write, 1)
void *ggl_arena_alloc(GglArena *arena, size_t size, size_t alignment);

/// Resize the last allocated ptr.
/// `ptr` must be the most recent allocation from `arena`.
/// `old_size` must match the original allocation size.
/// Returns GGL_ERR_OK on success, GGL_ERR_NOMEM if insufficient space,
/// or GGL_ERR_INVALID if validation fails.
NONNULL(2) ACCESS(read_write, 1) ACCESS(none, 2)
GglError ggl_arena_resize_last(
    GglArena arena[static 1], const void *ptr, size_t old_size, size_t size
);

/// Check if arena's memory region contains ptr.
/// Returns true if `ptr` is within the arena's memory range.
PURE ACCESS(read_only, 1) ACCESS(none, 2)
bool ggl_arena_owns(const GglArena *arena, const void *ptr);

/// Allocate all remaining space in the arena as a buffer.
/// Returns a buffer containing all unallocated space.
ACCESS(read_write, 1)
GglBuffer ggl_arena_alloc_rest(GglArena *arena);

/// Modify an object's references to point into an arena.
/// Copies buffers, lists, and maps that are not already in `arena`.
/// Updates `obj` in place to reference the copied data.
/// Returns GGL_ERR_OK on success, GGL_ERR_NOMEM if insufficient space.
ACCESS(read_write, 1) ACCESS(read_write, 2)
GglError ggl_arena_claim_obj(GglObject obj[static 1], GglArena *arena);

/// Modify a buffer to point into an arena.
/// Copies buffer data if not already in `arena`.
/// Updates `buf` in place to reference the copied data.
/// Returns GGL_ERR_OK on success, GGL_ERR_NOMEM if insufficient space.
ACCESS(read_write, 1) ACCESS(read_write, 2)
GglError ggl_arena_claim_buf(GglBuffer buf[static 1], GglArena *arena);

/// Modify buffer references in an object to point into an arena.
/// Copies buffers and map keys that are not already in `arena`.
/// Does not copy list or map memory.
/// Updates buffer pointers in place throughout the object tree.
/// Returns GGL_ERR_OK on success, GGL_ERR_NOMEM if insufficient space.
ACCESS(read_write, 1) ACCESS(read_write, 2)
GglError ggl_arena_claim_obj_bufs(GglObject obj[static 1], GglArena *arena);

#endif
