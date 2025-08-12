// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_FLAGS_H
#define GGL_FLAGS_H

//! Generic named flag types

#include <ggl/attr.h>

typedef struct DESIGNATED_INIT {
    enum {
        GGL_PRESENCE_REQUIRED,
        GGL_PRESENCE_OPTIONAL,
        GGL_PRESENCE_MISSING
    } val;
} GglPresence;

#define GGL_REQUIRED \
    (GglPresence) { \
        .val = GGL_PRESENCE_REQUIRED \
    }
#define GGL_OPTIONAL \
    (GglPresence) { \
        .val = GGL_PRESENCE_OPTIONAL \
    }
#define GGL_MISSING \
    (GglPresence) { \
        .val = GGL_PRESENCE_MISSING \
    }

#endif
