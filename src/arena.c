// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <ggl/arena.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <ggl/object_visit.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// NOLINTNEXTLINE(readability-redundant-declaration)
extern inline typeof(ggl_arena_init) ggl_arena_init;

void *ggl_arena_alloc(GglArena *arena, size_t size, size_t alignment) {
    if (arena == NULL) {
        GGL_LOGD("Attempted allocation with NULL arena.");
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
        GGL_LOGD("[%p] Need %" PRIu32 " padding.", arena, pad);
    }

    if (pad > arena->capacity - arena->index) {
        GGL_LOGD(
            "[%p] Insufficient memory for padding; returning NULL.", arena
        );
        return NULL;
    }

    uint32_t idx = arena->index + pad;

    if (size > arena->capacity - idx) {
        GGL_LOGD(
            "[%p] Insufficient memory to alloc %zu; returning NULL.",
            arena,
            size + pad
        );
        return NULL;
    }

    arena->index = idx + (uint32_t) size;
    return &arena->mem[idx];
}

GglError ggl_arena_resize_last(
    GglArena arena[static 1], const void *ptr, size_t old_size, size_t size
) {
    assert(arena != NULL);
    assert(ptr != NULL);
    assert(old_size < UINT32_MAX);
    assert(old_size <= PTRDIFF_MAX);
    assert(size <= PTRDIFF_MAX);

    if (!ggl_arena_owns(arena, ptr)) {
        GGL_LOGE("[%p] Resize ptr %p not owned.", arena, ptr);
        assert(false);
        return GGL_ERR_INVALID;
    }

    uint32_t idx = (uint32_t) ((uintptr_t) ptr - (uintptr_t) arena->mem);

    if (idx > arena->index) {
        GGL_LOGE("[%p] Resize ptr %p out of allocated range.", arena, ptr);
        assert(false);
        return GGL_ERR_INVALID;
    }

    if (arena->index - idx != old_size) {
        GGL_LOGE(
            "[%p] Resize ptr %p + size %zu does not match allocation index",
            arena,
            ptr,
            old_size
        );
        return GGL_ERR_INVALID;
    }

    if (size > arena->capacity - idx) {
        GGL_LOGD(
            "[%p] Insufficient memory to resize %p to %zu.", arena, ptr, size
        );
        return GGL_ERR_NOMEM;
    }

    arena->index = idx + (uint32_t) size;
    return GGL_ERR_OK;
}

bool ggl_arena_owns(const GglArena *arena, const void *ptr) {
    if (arena == NULL) {
        return false;
    }
    uintptr_t mem_int = (uintptr_t) arena->mem;
    uintptr_t ptr_int = (uintptr_t) ptr;
    return (ptr_int >= mem_int) && (ptr_int < mem_int + arena->capacity);
}

GglBuffer ggl_arena_alloc_rest(GglArena *arena) {
    if (arena == NULL) {
        return (GglBuffer) { 0 };
    }
    size_t remaining = arena->capacity - arena->index;
    uint8_t *data = GGL_ARENA_ALLOCN(arena, uint8_t, remaining);
    assert(data != NULL);
    return (GglBuffer) { .data = data, .len = remaining };
}

GglError ggl_arena_claim_buf(GglBuffer buf[static 1], GglArena *arena) {
    assert(buf != NULL);

    if (ggl_arena_owns(arena, buf->data)) {
        return GGL_ERR_OK;
    }

    if (buf->len == 0) {
        buf->data = NULL;
        return GGL_ERR_OK;
    }

    uint8_t *new_mem = GGL_ARENA_ALLOCN(arena, uint8_t, buf->len);
    if (new_mem == NULL) {
        GGL_LOGE("Insufficient memory when cloning buffer into %p.", arena);
        return GGL_ERR_NOMEM;
    }

    memcpy(new_mem, buf->data, buf->len);
    buf->data = new_mem;
    return GGL_ERR_OK;
}

static GglError claim_buf(void *ctx, GglBuffer val, GglObject obj[static 1]) {
    GglArena *arena = ctx;
    GglError ret = ggl_arena_claim_buf(&val, arena);
    *obj = ggl_obj_buf(val);
    return ret;
}

static GglError claim_list(void *ctx, GglList val, GglObject obj[static 1]) {
    GglArena *arena = ctx;
    if (ggl_arena_owns(arena, val.items)) {
        return GGL_ERR_OK;
    }
    if (val.len == 0) {
        *obj = ggl_obj_list((GglList) { 0 });
        return GGL_ERR_OK;
    }
    GglObject *new_mem = GGL_ARENA_ALLOCN(arena, GglObject, val.len);
    if (new_mem == NULL) {
        GGL_LOGE("Insufficient memory when cloning list into %p.", arena);
        return GGL_ERR_NOMEM;
    }
    memcpy(new_mem, val.items, val.len * sizeof(GglObject));
    *obj = ggl_obj_list((GglList) { .items = new_mem, .len = val.len });
    return GGL_ERR_OK;
}

static GglError claim_map(void *ctx, GglMap val, GglObject obj[static 1]) {
    GglArena *arena = ctx;
    if (ggl_arena_owns(arena, val.pairs)) {
        return GGL_ERR_OK;
    }
    if (val.len == 0) {
        *obj = ggl_obj_map((GglMap) { 0 });
        val.pairs = NULL;
        return GGL_ERR_OK;
    }

    GglKV *new_mem = GGL_ARENA_ALLOCN(arena, GglKV, val.len);
    if (new_mem == NULL) {
        GGL_LOGE("Insufficient memory when cloning map into %p.", arena);
        return GGL_ERR_NOMEM;
    }
    memcpy(new_mem, val.pairs, val.len * sizeof(GglKV));
    *obj = ggl_obj_map((GglMap) { .pairs = new_mem, .len = val.len });
    return GGL_ERR_OK;
}

static GglError claim_map_key(void *ctx, GglBuffer key, GglKV kv[static 1]) {
    GglArena *arena = ctx;
    GglError ret = ggl_arena_claim_buf(&key, arena);
    ggl_kv_set_key(kv, key);
    return ret;
}

GglError ggl_arena_claim_obj(GglObject obj[static 1], GglArena *arena) {
    const GglObjectVisitHandlers VISIT_HANDLERS = {
        .on_buf = claim_buf,
        .on_list = claim_list,
        .on_map = claim_map,
        .on_map_key = claim_map_key,
    };
    return ggl_obj_visit(&VISIT_HANDLERS, arena, obj);
}

GglError ggl_arena_claim_obj_bufs(GglObject obj[static 1], GglArena *arena) {
    const GglObjectVisitHandlers VISIT_HANDLERS = {
        .on_buf = claim_buf,
        .on_map_key = claim_map_key,
    };
    return ggl_obj_visit(&VISIT_HANDLERS, arena, obj);
}
