// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_OBJECT_VISIT_H
#define GG_OBJECT_VISIT_H

//! Non-recursive iteration helper for GgObjects

#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/object.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    GgError (*on_null)(void *ctx);
    GgError (*on_bool)(void *ctx, bool val);
    GgError (*on_i64)(void *ctx, int64_t val);
    GgError (*on_f64)(void *ctx, double val);
    GgError (*on_buf)(void *ctx, GgBuffer val, GgObject obj[static 1]);
    GgError (*on_list)(void *ctx, GgList val, GgObject obj[static 1]);
    GgError (*cont_list)(void *ctx);
    GgError (*end_list)(void *ctx);
    GgError (*on_map)(void *ctx, GgMap val, GgObject obj[static 1]);
    GgError (*on_map_key)(void *ctx, GgBuffer key, GgKV kv[static 1]);
    GgError (*cont_map)(void *ctx);
    GgError (*end_map)(void *ctx);
} GgObjectVisitHandlers;

VISIBILITY(hidden)
GgError gg_obj_visit(
    const GgObjectVisitHandlers handlers[static 1],
    void *ctx,
    GgObject obj[static 1]
);

#endif
