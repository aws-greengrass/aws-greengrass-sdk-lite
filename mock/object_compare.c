#include "gg/object_compare.h"
#include <float.h>
#include <gg/buffer.h>
#include <gg/log.h>
#include <gg/map.h>
#include <gg/object.h>
#include <gg/object_iter.h>
#include <inttypes.h>
#include <math.h>

/// Prints info about all parents of the current subobject.
static void print_state(const IterLevels state[static 1]) {
    for (uint8_t i = state->index; i > 0; --i) {
        switch (state->state[i - 1]) {
        case LEVEL_LIST: {
            GG_LOGE("In list (idx = %d).", (int) state->elem_index[i]);
        } break;
        case LEVEL_MAP: {
            GgMap map = gg_obj_into_map(*state->obj[i]);
            GgBuffer key = gg_kv_key(map.pairs[state->elem_index[i]]);
            GG_LOGE("In map (key = \"%.*s\").", (int) key.len, key.data);
        } break;
        default:
            return;
        }
    }
}

static bool bool_eq(bool lhs, bool rhs) {
    if (lhs != rhs) {
        GG_LOGE(
            "Bool comparison failed (%s != %s).",
            lhs ? " true" : "false",
            rhs ? " true" : "false"
        );
        return false;
    }
    return true;
}

static bool int_eq(int64_t lhs, int64_t rhs) {
    if (lhs != rhs) {
        GG_LOGE(
            "Int comparison failed (%" PRIi64 " != %" PRIi64 ").", lhs, rhs
        );
        return false;
    }
    return true;
}

static bool float_eq(double lhs, double rhs) {
    if (isnan(lhs) != isnan(rhs)) {
        GG_LOGE("NaN comparison failed (%g != %g)", lhs, rhs);
        return false;
    }
    double error = fabs(lhs - rhs);
    if (error <= DBL_EPSILON) {
        return true;
    }
    if (error <= (DBL_EPSILON * fmin(fabs(lhs), fabs(rhs)))) {
        return true;
    }
    GG_LOGE("Float comparison failed (%g != %g).", lhs, rhs);
    return false;
}

static bool buf_eq(GgBuffer lhs, GgBuffer rhs) {
    if (!gg_buffer_eq(lhs, rhs)) {
        GG_LOGE(
            "Buffer comparison failed (\"%.*s\" != \"%.*s\").",
            (int) lhs.len,
            lhs.data,
            (int) rhs.len,
            rhs.data
        );
        return false;
    }
    return true;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
bool gg_obj_eq(GgObject lhs, GgObject rhs) {
    IterLevels lhs_state;
    IterLevels rhs_state;

    lhs_state.index = 0;
    lhs_state.obj[0] = &lhs;
    lhs_state.state[0] = LEVEL_DEFAULT;
    lhs_state.elem_index[0] = 0;

    rhs_state.index = 0;
    rhs_state.obj[0] = &rhs;
    // lhs state array is used for both iter levels
    rhs_state.elem_index[0] = 0;

    uint8_t subobjects = 0;

    while (true) {
        GgObject *lhs_obj = lhs_state.obj[lhs_state.index];
        GgObject *rhs_obj = rhs_state.obj[rhs_state.index];

        IterLevelState cur_state = lhs_state.state[lhs_state.index];
        uint8_t lhs_index = lhs_state.elem_index[lhs_state.index];
        uint8_t rhs_index = rhs_state.elem_index[rhs_state.index];

        switch (cur_state) {
        case LEVEL_DEFAULT: {
            if (gg_obj_type(*lhs_obj) != gg_obj_type(*rhs_obj)) {
                print_state(&lhs_state);
                return false;
            }

            switch (gg_obj_type(*lhs_obj)) {
            case GG_TYPE_NULL:
                break;
            case GG_TYPE_BOOLEAN:
                if (!bool_eq(
                        gg_obj_into_bool(*lhs_obj), gg_obj_into_bool(*rhs_obj)
                    )) {
                    print_state(&lhs_state);
                    return false;
                }
                break;
            case GG_TYPE_I64:
                if (!int_eq(
                        gg_obj_into_i64(*lhs_obj), gg_obj_into_i64(*rhs_obj)
                    )) {
                    print_state(&lhs_state);
                    return false;
                }
                break;
            case GG_TYPE_F64:
                if (!float_eq(
                        gg_obj_into_f64(*lhs_obj), gg_obj_into_f64(*rhs_obj)
                    )) {
                    return false;
                }
                break;
            case GG_TYPE_BUF:
                if (!buf_eq(
                        gg_obj_into_buf(*lhs_obj), gg_obj_into_buf(*rhs_obj)
                    )) {
                    print_state(&lhs_state);
                    return false;
                }
                break;
            case GG_TYPE_LIST: {
                GgList lhs_list = gg_obj_into_list(*lhs_obj);
                if (lhs_list.len > GG_MAX_OBJECT_SUBOBJECTS - subobjects) {
                    GG_LOGE("Visited object's subobjects exceeds maximum.");
                    print_state(&lhs_state);
                    return false;
                }
                subobjects += (uint8_t) lhs_list.len;
                GgList rhs_list = gg_obj_into_list(*rhs_obj);
                if (lhs_list.len != rhs_list.len) {
                    GG_LOGE("List length mismatch.");
                    print_state(&lhs_state);
                    return false;
                }

                lhs_state.state[lhs_state.index] = LEVEL_LIST;
                continue;
            }
            case GG_TYPE_MAP: {
                GgMap lhs_map = gg_obj_into_map(*lhs_obj);
                if (lhs_map.len > (GG_MAX_OBJECT_SUBOBJECTS - subobjects) / 2) {
                    GG_LOGE("Visited object's subobjects exceeds maximum.");
                    print_state(&lhs_state);
                    return false;
                }
                subobjects += (uint8_t) (lhs_map.len * 2);

                GgMap rhs_map = gg_obj_into_map(*rhs_obj);
                if (lhs_map.len != rhs_map.len) {
                    GG_LOGE("Map length mismatch.");
                    print_state(&lhs_state);
                    return false;
                }

                lhs_state.state[lhs_state.index] = LEVEL_MAP;
                continue;
            }
            }
        } break;
        case LEVEL_LIST: {
            GgList lhs_list = gg_obj_into_list(*lhs_obj);
            if (lhs_index == lhs_list.len) {
                break;
            }
            GgList rhs_list = gg_obj_into_list(*rhs_obj);

            if (lhs_state.index >= GG_MAX_OBJECT_DEPTH - 1) {
                GG_LOGE("Visited object's depth exceeds maximum.");
                print_state(&lhs_state);
                return false;
            }
            lhs_state.index += 1;
            rhs_state.index += 1;

            lhs_state.obj[lhs_state.index] = &lhs_list.items[lhs_index];
            lhs_state.state[lhs_state.index] = LEVEL_DEFAULT;
            lhs_state.elem_index[lhs_state.index] = 0;

            lhs_state.elem_index[lhs_state.index - 1] += 1;

            rhs_state.obj[rhs_state.index] = &rhs_list.items[rhs_index];
            rhs_state.elem_index[rhs_state.index] = 0;

            rhs_state.elem_index[rhs_state.index - 1] += 1;
            continue;
        }
        case LEVEL_MAP: {
            GgMap lhs_map = gg_obj_into_map(*lhs_obj);
            if (lhs_index == lhs_map.len) {
                break;
            }
            GgMap rhs_map = gg_obj_into_map(*rhs_obj);

            GgKV *kv = &lhs_map.pairs[lhs_index];

            // GgMap is unordered, so each rhs key must be found
            GgObject *rhs_val = NULL;
            GgBuffer key = gg_kv_key(*kv);
            if (!gg_map_get(rhs_map, key, &rhs_val)) {
                GG_LOGE("Map key %.*s not found.", (int) key.len, key.data);
                print_state(&lhs_state);
                return false;
            }

            if (lhs_state.index >= GG_MAX_OBJECT_DEPTH - 1) {
                GG_LOGE("Visited object's depth exceeds maximum.");
                print_state(&lhs_state);
                return false;
            }
            lhs_state.index += 1;
            rhs_state.index += 1;

            lhs_state.obj[lhs_state.index] = gg_kv_val(kv);
            lhs_state.state[lhs_state.index] = LEVEL_DEFAULT;
            lhs_state.elem_index[lhs_state.index] = 0;

            lhs_state.elem_index[lhs_state.index - 1] += 1;

            rhs_state.obj[rhs_state.index] = rhs_val;
            rhs_state.elem_index[rhs_state.index] = 0;

            rhs_state.elem_index[rhs_state.index - 1]
                = (uint8_t) (kv - rhs_map.pairs);

            continue;
        }
        }

        if (lhs_state.index == 0) {
            break;
        }

        lhs_state.index -= 1;
        rhs_state.index -= 1;
    }

    return true;
}
