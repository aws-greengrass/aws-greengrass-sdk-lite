// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_ALLOC_H
#define GG_ALLOC_H

//! Generic allocator interface

#include <gg/attr.h>
#include <stdalign.h>
#include <stddef.h>

/// Allocator vtable.
typedef struct {
    void *(*const ALLOC)(void *ctx, size_t size, size_t alignment);
    void (*const FREE)(void *ctx, void *ptr);
} DESIGNATED_INIT GgAllocVtable;

typedef struct {
    const GgAllocVtable *const VTABLE;
    void *ctx;
} DESIGNATED_INIT GgAlloc;

/// Allocate a single `type` from an allocator.
#define GG_ALLOC(alloc, type) \
    (typeof(type) *) gg_alloc(alloc, sizeof(type), alignof(type))

/// Allocate `n` units of `type` from an allocator.
#define GG_ALLOCN(alloc, type, n) \
    (typeof(type) *) gg_alloc(alloc, (n) * sizeof(type), alignof(type))

/// Allocate memory from an allocator.
/// Prefer `GG_ALLOC` or `GG_ALLOCN`.
VISIBILITY(hidden)
void *gg_alloc(GgAlloc alloc, size_t size, size_t alignment);

/// Free memory allocated from an allocator.
VISIBILITY(hidden)
void gg_free(GgAlloc alloc, void *ptr);

#endif
