// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <gg/arena.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/log.h>
#include <gg/map.h>
#include <gg/object.h>
#include <gg/object_visit.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// NOLINTNEXTLINE(readability-redundant-declaration)
extern inline typeof(gg_arena_init) gg_arena_init;

void *gg_arena_alloc(GgArena *arena, size_t size, size_t alignment) {
    if (arena == NULL) {
        GG_LOGD("Attempted allocation with NULL arena.");
        return NULL;
    }

    // alignment must be power of 2
    assert((alignment > 0) && ((alignment & (alignment - 1)) == 0));
    assert(alignment < UINT32_MAX);
    // Allocation can't exceed ptrdiff_t, and this is likely a bug.
    assert(size <= PTRDIFF_MAX);

    uint32_t align = (uint32_t) alignment;
    uint32_t pad = (align - (arena->index & (align - 1))) & (align - 1);

    if (pad > 0) {
        GG_LOGD("[%p] Need %" PRIu32 " padding.", arena, pad);
    }

    if (pad > arena->capacity - arena->index) {
        GG_LOGD("[%p] Insufficient memory for padding; returning NULL.", arena);
        return NULL;
    }

    uint32_t idx = arena->index + pad;

    if (size > arena->capacity - idx) {
        GG_LOGD(
            "[%p] Insufficient memory to alloc %zu; returning NULL.",
            arena,
            size + pad
        );
        return NULL;
    }

    arena->index = idx + (uint32_t) size;
    return &arena->mem[idx];
}

GgError gg_arena_resize_last(
    GgArena arena[static 1], const void *ptr, size_t old_size, size_t size
) {
    assert(arena != NULL);
    assert(ptr != NULL);
    assert(old_size < UINT32_MAX);
    assert(old_size <= PTRDIFF_MAX);
    assert(size <= PTRDIFF_MAX);

    if (!gg_arena_owns(arena, ptr)) {
        GG_LOGE("[%p] Resize ptr %p not owned.", arena, ptr);
        assert(false);
        return GG_ERR_INVALID;
    }

    uint32_t idx = (uint32_t) ((uintptr_t) ptr - (uintptr_t) arena->mem);

    if (idx > arena->index) {
        GG_LOGE("[%p] Resize ptr %p out of allocated range.", arena, ptr);
        assert(false);
        return GG_ERR_INVALID;
    }

    if (arena->index - idx != old_size) {
        GG_LOGE(
            "[%p] Resize ptr %p + size %zu does not match allocation index",
            arena,
            ptr,
            old_size
        );
        return GG_ERR_INVALID;
    }

    if (size > arena->capacity - idx) {
        GG_LOGD(
            "[%p] Insufficient memory to resize %p to %zu.", arena, ptr, size
        );
        return GG_ERR_NOMEM;
    }

    arena->index = idx + (uint32_t) size;
    return GG_ERR_OK;
}

bool gg_arena_owns(const GgArena *arena, const void *ptr) {
    if (arena == NULL) {
        return false;
    }
    uintptr_t mem_int = (uintptr_t) arena->mem;
    uintptr_t ptr_int = (uintptr_t) ptr;
    return (ptr_int >= mem_int) && (ptr_int < mem_int + arena->capacity);
}

GgBuffer gg_arena_alloc_rest(GgArena *arena) {
    if (arena == NULL) {
        return (GgBuffer) { 0 };
    }
    size_t remaining = arena->capacity - arena->index;
    uint8_t *data = GG_ARENA_ALLOCN(arena, uint8_t, remaining);
    assert(data != NULL);
    return (GgBuffer) { .data = data, .len = remaining };
}

GgError gg_arena_claim_buf(GgBuffer buf[static 1], GgArena *arena) {
    assert(buf != NULL);

    if (gg_arena_owns(arena, buf->data)) {
        return GG_ERR_OK;
    }

    if (buf->len == 0) {
        buf->data = NULL;
        return GG_ERR_OK;
    }

    uint8_t *new_mem = GG_ARENA_ALLOCN(arena, uint8_t, buf->len);
    if (new_mem == NULL) {
        GG_LOGE("Insufficient memory when cloning buffer into %p.", arena);
        return GG_ERR_NOMEM;
    }

    memcpy(new_mem, buf->data, buf->len);
    buf->data = new_mem;
    return GG_ERR_OK;
}

static GgError claim_buf(void *ctx, GgBuffer val, GgObject obj[static 1]) {
    GgArena *arena = ctx;
    GgError ret = gg_arena_claim_buf(&val, arena);
    *obj = gg_obj_buf(val);
    return ret;
}

static GgError claim_list(void *ctx, GgList val, GgObject obj[static 1]) {
    GgArena *arena = ctx;
    if (gg_arena_owns(arena, val.items)) {
        return GG_ERR_OK;
    }
    if (val.len == 0) {
        *obj = gg_obj_list((GgList) { 0 });
        return GG_ERR_OK;
    }
    GgObject *new_mem = GG_ARENA_ALLOCN(arena, GgObject, val.len);
    if (new_mem == NULL) {
        GG_LOGE("Insufficient memory when cloning list into %p.", arena);
        return GG_ERR_NOMEM;
    }
    memcpy(new_mem, val.items, val.len * sizeof(GgObject));
    *obj = gg_obj_list((GgList) { .items = new_mem, .len = val.len });
    return GG_ERR_OK;
}

static GgError claim_map(void *ctx, GgMap val, GgObject obj[static 1]) {
    GgArena *arena = ctx;
    if (gg_arena_owns(arena, val.pairs)) {
        return GG_ERR_OK;
    }
    if (val.len == 0) {
        *obj = gg_obj_map((GgMap) { 0 });
        val.pairs = NULL;
        return GG_ERR_OK;
    }

    GgKV *new_mem = GG_ARENA_ALLOCN(arena, GgKV, val.len);
    if (new_mem == NULL) {
        GG_LOGE("Insufficient memory when cloning map into %p.", arena);
        return GG_ERR_NOMEM;
    }
    memcpy(new_mem, val.pairs, val.len * sizeof(GgKV));
    *obj = gg_obj_map((GgMap) { .pairs = new_mem, .len = val.len });
    return GG_ERR_OK;
}

static GgError claim_map_key(void *ctx, GgBuffer key, GgKV kv[static 1]) {
    GgArena *arena = ctx;
    GgError ret = gg_arena_claim_buf(&key, arena);
    gg_kv_set_key(kv, key);
    return ret;
}

GgError gg_arena_claim_obj(GgObject obj[static 1], GgArena *arena) {
    const GgObjectVisitHandlers VISIT_HANDLERS = {
        .on_buf = claim_buf,
        .on_list = claim_list,
        .on_map = claim_map,
        .on_map_key = claim_map_key,
    };
    return gg_obj_visit(&VISIT_HANDLERS, arena, obj);
}

GgError gg_arena_claim_obj_bufs(GgObject obj[static 1], GgArena *arena) {
    const GgObjectVisitHandlers VISIT_HANDLERS = {
        .on_buf = claim_buf,
        .on_map_key = claim_map_key,
    };
    return gg_obj_visit(&VISIT_HANDLERS, arena, obj);
}
