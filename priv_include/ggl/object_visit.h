// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_OBJECT_VISIT_H
#define GGL_OBJECT_VISIT_H

//! Non-recursive iteration helper for GglObjects

#include <ggl/attr.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/object.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    GglError (*on_null)(void *ctx);
    GglError (*on_bool)(void *ctx, bool val);
    GglError (*on_i64)(void *ctx, int64_t val);
    GglError (*on_f64)(void *ctx, double val);
    GglError (*on_buf)(void *ctx, GglBuffer val, GglObject obj[static 1]);
    GglError (*on_list)(void *ctx, GglList val, GglObject obj[static 1]);
    GglError (*cont_list)(void *ctx);
    GglError (*end_list)(void *ctx);
    GglError (*on_map)(void *ctx, GglMap val, GglObject obj[static 1]);
    GglError (*on_map_key)(void *ctx, GglBuffer key, GglKV kv[static 1]);
    GglError (*cont_map)(void *ctx);
    GglError (*end_map)(void *ctx);
} GglObjectVisitHandlers;

VISIBILITY(hidden)
GglError ggl_obj_visit(
    const GglObjectVisitHandlers handlers[static 1],
    void *ctx,
    GglObject obj[static 1]
);

#endif
