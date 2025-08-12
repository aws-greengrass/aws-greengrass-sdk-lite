// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_INIT_H
#define GGL_INIT_H

#include <ggl/attr.h>
#include <ggl/error.h>

typedef struct GglInitEntry {
    GglError (*fn)(void);
    struct GglInitEntry *next;
} GglInitEntry;

/// Initializes the sdk, including starting necessary threads.
/// Unused portions of sdk may not be initialized.
/// Not thread-safe
VISIBILITY(hidden)
void ggl_register_init_fn(GglInitEntry entry[static 1]);

#endif
