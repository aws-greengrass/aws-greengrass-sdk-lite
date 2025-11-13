// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_INIT_H
#define GG_INIT_H

#include <gg/attr.h>
#include <gg/error.h>

typedef struct GgInitEntry {
    GgError (*fn)(void);
    struct GgInitEntry *next;
} GgInitEntry;

/// Initializes the sdk, including starting necessary threads.
/// Unused portions of sdk may not be initialized.
/// Not thread-safe
VISIBILITY(hidden)
void gg_register_init_fn(GgInitEntry entry[static 1]);

#endif
