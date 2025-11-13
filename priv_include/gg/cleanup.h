// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_CLEANUP_H
#define GG_CLEANUP_H

//! Macros for automatic resource cleanup

#include <gg/macro_util.h>
#include <pthread.h>
#include <stdlib.h>

#define GG_CLEANUP_ID(ident, fn, exp) \
    __attribute__((cleanup(fn))) typeof(exp) ident = (exp);
#define GG_CLEANUP(fn, exp) \
    GG_CLEANUP_ID(GG_MACRO_PASTE(cleanup_, __LINE__), fn, exp)

// CBMC errors if another function in the compilation object calls free
#ifndef __CPROVER__

static inline void cleanup_free(void *p) {
    free(*(void **) p);
}

#endif

static inline void cleanup_pthread_mtx_unlock(pthread_mutex_t **mtx) {
    if (*mtx != NULL) {
        pthread_mutex_unlock(*mtx);
    }
}

// NOLINTBEGIN(bugprone-macro-parentheses)
#define GG_MTX_SCOPE_GUARD_ID(ident, mtx) \
    __attribute__((cleanup(cleanup_pthread_mtx_unlock))) \
    pthread_mutex_t *ident \
        = mtx; \
    pthread_mutex_lock(ident);
// NOLINTEND(bugprone-macro-parentheses)

#define GG_MTX_SCOPE_GUARD(mtx) \
    GG_MTX_SCOPE_GUARD_ID(GG_MACRO_PASTE(gg_mtx_lock_, __LINE__), mtx)

#endif
