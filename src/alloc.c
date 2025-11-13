// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/alloc.h>
#include <gg/log.h>
#include <stddef.h>

void *gg_alloc(GgAlloc alloc, size_t size, size_t alignment) {
    void *ret = NULL;

    if ((alloc.VTABLE != NULL) && (alloc.VTABLE->ALLOC != NULL)) {
        ret = alloc.VTABLE->ALLOC(alloc.ctx, size, alignment);
    }

    if (ret == NULL) {
        GG_LOGW(
            "[%p:%p] Failed alloc %zu bytes.",
            (void *) alloc.VTABLE,
            (void *) alloc.ctx,
            size
        );
    } else {
        GG_LOGT(
            "[%p:%p] alloc %p, len %zu.",
            (void *) alloc.VTABLE,
            (void *) alloc.ctx,
            ret,
            size
        );
    }

    return ret;
}

void gg_free(GgAlloc alloc, void *ptr) {
    GG_LOGT("[%p:%p] Free %p", (void *) alloc.VTABLE, (void *) alloc.ctx, ptr);

    if ((ptr != NULL) && (alloc.VTABLE != NULL)
        && (alloc.VTABLE->FREE != NULL)) {
        alloc.VTABLE->FREE(alloc.ctx, ptr);
    }
}
