// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_FLAGS_H
#define GG_FLAGS_H

//! Generic named flag types

#include <gg/attr.h>

typedef struct DESIGNATED_INIT {
    enum {
        GG_PRESENCE_REQUIRED,
        GG_PRESENCE_OPTIONAL,
        GG_PRESENCE_MISSING
    } val;
} GgPresence;

#define GG_REQUIRED \
    (GgPresence) { \
        .val = GG_PRESENCE_REQUIRED \
    }
#define GG_OPTIONAL \
    (GgPresence) { \
        .val = GG_PRESENCE_OPTIONAL \
    }
#define GG_MISSING \
    (GgPresence) { \
        .val = GG_PRESENCE_MISSING \
    }

#endif
