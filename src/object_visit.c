// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <ggl/error.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <ggl/object_visit.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static_assert(
    GGL_MAX_OBJECT_DEPTH <= UINT8_MAX,
    "GGL_MAX_OBJECT_DEPTH must fit in a uint8_t."
);
static_assert(
    GGL_MAX_OBJECT_SUBOBJECTS <= UINT8_MAX,
    "GGL_MAX_OBJECT_SUBOBJECTS must fit in a uint8_t."
);

typedef enum {
    LEVEL_DEFAULT,
    LEVEL_LIST,
    LEVEL_MAP,
} IterLevelState;

typedef struct {
    GglObject *obj[GGL_MAX_OBJECT_DEPTH];
    uint8_t state[GGL_MAX_OBJECT_DEPTH];
    uint8_t elem_index[GGL_MAX_OBJECT_DEPTH];
    uint8_t index;
} IterLevels;

#define TRY_HANDLER(name, ...) \
    if (handlers->name != NULL) { \
        GglError handler_ret = handlers->name(__VA_ARGS__); \
        if (handler_ret != GGL_ERR_OK) { \
            return handler_ret; \
        } \
    }

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
GglError ggl_obj_visit(
    const GglObjectVisitHandlers handlers[static 1],
    void *ctx,
    GglObject obj[static 1]
) {
    IterLevels state;

    state.index = 0;
    state.obj[0] = obj;
    state.state[0] = LEVEL_DEFAULT;
    state.elem_index[0] = 0;

    uint8_t subobjects = 0;

    while (true) {
        GglObject *cur_obj = state.obj[state.index];
        IterLevelState cur_state = state.state[state.index];
        uint8_t cur_index = state.elem_index[state.index];

        switch (cur_state) {
        case LEVEL_DEFAULT: {
            switch (ggl_obj_type(*cur_obj)) {
            case GGL_TYPE_NULL:
                TRY_HANDLER(on_null, ctx);
                break;
            case GGL_TYPE_BOOLEAN:
                TRY_HANDLER(on_bool, ctx, ggl_obj_into_bool(*cur_obj));
                break;
            case GGL_TYPE_I64:
                TRY_HANDLER(on_i64, ctx, ggl_obj_into_i64(*cur_obj));
                break;
            case GGL_TYPE_F64:
                TRY_HANDLER(on_f64, ctx, ggl_obj_into_f64(*cur_obj));
                break;
            case GGL_TYPE_BUF:
                TRY_HANDLER(on_buf, ctx, ggl_obj_into_buf(*cur_obj), cur_obj);
                break;
            case GGL_TYPE_LIST: {
                GglList list = ggl_obj_into_list(*cur_obj);
                if (list.len > GGL_MAX_OBJECT_SUBOBJECTS - subobjects) {
                    GGL_LOGE("Visited object's subobjects exceeds maximum.");
                    return GGL_ERR_RANGE;
                }
                subobjects += (uint8_t) list.len;

                TRY_HANDLER(on_list, ctx, list, cur_obj);
                state.state[state.index] = LEVEL_LIST;
                continue;
            }
            case GGL_TYPE_MAP: {
                GglMap map = ggl_obj_into_map(*cur_obj);
                if (map.len > (GGL_MAX_OBJECT_SUBOBJECTS - subobjects) / 2) {
                    GGL_LOGE("Visited object's subobjects exceeds maximum.");
                    return GGL_ERR_RANGE;
                }
                subobjects += (uint8_t) (map.len * 2);

                TRY_HANDLER(on_map, ctx, ggl_obj_into_map(*cur_obj), cur_obj);
                state.state[state.index] = LEVEL_MAP;
                continue;
            }
            }
        } break;
        case LEVEL_LIST: {
            GglList list = ggl_obj_into_list(*cur_obj);
            if (cur_index == list.len) {
                TRY_HANDLER(end_list, ctx);
                break;
            }

            if (cur_index != 0) {
                TRY_HANDLER(cont_list, ctx);
            }

            state.index += 1;
            if (state.index == GGL_MAX_OBJECT_DEPTH) {
                GGL_LOGE("Visited object's depth exceeds maximum.");
                return GGL_ERR_RANGE;
            }

            state.obj[state.index] = &list.items[cur_index];
            state.state[state.index] = LEVEL_DEFAULT;
            state.elem_index[state.index] = 0;

            state.elem_index[state.index - 1] += 1;
            continue;
        }
        case LEVEL_MAP: {
            GglMap map = ggl_obj_into_map(*cur_obj);
            if (cur_index == map.len) {
                TRY_HANDLER(end_map, ctx);
                break;
            }

            if (cur_index != 0) {
                TRY_HANDLER(cont_map, ctx);
            }

            GglKV *kv = &map.pairs[cur_index];
            TRY_HANDLER(on_map_key, ctx, ggl_kv_key(*kv), kv);

            state.index += 1;
            if (state.index == GGL_MAX_OBJECT_DEPTH) {
                GGL_LOGE("Visited object's depth exceeds maximum.");
                return GGL_ERR_RANGE;
            }

            state.obj[state.index] = ggl_kv_val(kv);
            state.state[state.index] = LEVEL_DEFAULT;
            state.elem_index[state.index] = 0;

            state.elem_index[state.index - 1] += 1;
            continue;
        }
        }

        if (state.index == 0) {
            break;
        }

        state.index -= 1;
    }

    return GGL_ERR_OK;
}
