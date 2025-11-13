// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_ARENA_H
#define GG_ARENA_H

//! Arena allocation

#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/object.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Arena allocator backed by fixed buffer.
typedef struct {
    uint8_t *mem;
    uint32_t capacity;
    uint32_t index;
} GgArena;

/// Saved state of an arena allocator.
typedef struct {
    uint32_t index;
} GgArenaState;

/// Obtain an initialized `GgAlloc` backed by `buf`.
inline GgArena gg_arena_init(GgBuffer buf) {
    return (GgArena) { .mem = buf.data,
                       .capacity = buf.len <= UINT32_MAX ? (uint32_t) buf.len
                                                         : UINT32_MAX };
}

/// Allocate a `type` from an arena.
#define GG_ARENA_ALLOC(arena, type) \
    (typeof(type) *) gg_arena_alloc(arena, sizeof(type), alignof(type))
/// Allocate `n` units of `type` from an arena.
#define GG_ARENA_ALLOCN(arena, type, n) \
    (typeof(type) *) gg_arena_alloc(arena, (n) * sizeof(type), alignof(type))

/// Allocate `size` bytes with given alignment from an arena.
/// Returns pointer to allocated memory, or NULL if insufficient space.
/// Alignment must be a power of 2.
ACCESS(read_write, 1)
void *gg_arena_alloc(GgArena *arena, size_t size, size_t alignment);

/// Resize the last allocated ptr.
/// `ptr` must be the most recent allocation from `arena`.
/// `old_size` must match the original allocation size.
/// Returns GG_ERR_OK on success, GG_ERR_NOMEM if insufficient space,
/// or GG_ERR_INVALID if validation fails.
NONNULL(2) ACCESS(read_write, 1) ACCESS(none, 2)
GgError gg_arena_resize_last(
    GgArena arena[static 1], const void *ptr, size_t old_size, size_t size
);

/// Check if arena's memory region contains ptr.
/// Returns true if `ptr` is within the arena's memory range.
PURE ACCESS(read_only, 1) ACCESS(none, 2)
bool gg_arena_owns(const GgArena *arena, const void *ptr);

/// Allocate all remaining space in the arena as a buffer.
/// Returns a buffer containing all unallocated space.
ACCESS(read_write, 1)
GgBuffer gg_arena_alloc_rest(GgArena *arena);

/// Modify an object's references to point into an arena.
/// Copies buffers, lists, and maps that are not already in `arena`.
/// Updates `obj` in place to reference the copied data.
/// Returns GG_ERR_OK on success, GG_ERR_NOMEM if insufficient space.
ACCESS(read_write, 1) ACCESS(read_write, 2)
GgError gg_arena_claim_obj(GgObject obj[static 1], GgArena *arena);

/// Modify a buffer to point into an arena.
/// Copies buffer data if not already in `arena`.
/// Updates `buf` in place to reference the copied data.
/// Returns GG_ERR_OK on success, GG_ERR_NOMEM if insufficient space.
ACCESS(read_write, 1) ACCESS(read_write, 2)
GgError gg_arena_claim_buf(GgBuffer buf[static 1], GgArena *arena);

/// Modify buffer references in an object to point into an arena.
/// Copies buffers and map keys that are not already in `arena`.
/// Does not copy list or map memory.
/// Updates buffer pointers in place throughout the object tree.
/// Returns GG_ERR_OK on success, GG_ERR_NOMEM if insufficient space.
ACCESS(read_write, 1) ACCESS(read_write, 2)
GgError gg_arena_claim_obj_bufs(GgObject obj[static 1], GgArena *arena);

#endif
