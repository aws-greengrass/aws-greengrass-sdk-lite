// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/client.h>
#include <ggl/ipc/client_raw.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <ggl/vector.h>
#include <string.h>
#include <time.h>

struct timespec;

static GglError error_handler(
    void *ctx, GglBuffer error_code, GglBuffer message
) {
    (void) ctx;

    GGL_LOGE(
        "Received UpdateConfig error %.*s: %.*s.",
        (int) error_code.len,
        error_code.data,
        (int) message.len,
        message.data
    );

    return GGL_ERR_FAILURE;
}

GglError ggipc_update_config(
    GglBufList key_path,
    const struct timespec *timestamp,
    GglObject value_to_merge
) {
    if ((timestamp != NULL)
        && ((timestamp->tv_sec < 0) || (timestamp->tv_nsec < 0))) {
        GGL_LOGE("Timestamp is negative.");
        return GGL_ERR_UNSUPPORTED;
    }

    GglKV pair_to_merge;
    GglObjectType type = ggl_obj_type(value_to_merge);
    if (type != GGL_TYPE_MAP) {
        // For v2 compatibility, map the final key in the path to the
        // value_to_merge
        if (key_path.len == 0) {
            GGL_LOGE("Root configuration object must be a map.");
            return GGL_ERR_INVALID;
        }
        pair_to_merge = ggl_kv(key_path.bufs[key_path.len - 1], value_to_merge);
        value_to_merge = ggl_obj_map((GglMap) {
            .pairs = &pair_to_merge,
            .len = 1,
        });
        key_path.len -= 1;
    }

    GglObjVec path_vec
        = GGL_OBJ_VEC((GglObject[GGL_MAX_OBJECT_DEPTH - 1]) { 0 });
    GglError ret = GGL_ERR_OK;
    for (size_t i = 0; i < key_path.len; i++) {
        ggl_obj_vec_chain_push(&ret, &path_vec, ggl_obj_buf(key_path.bufs[i]));
    }
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Key path too long.");
        return GGL_ERR_NOMEM;
    }

    double timestamp_float = 0.0;
    if (timestamp != NULL) {
        timestamp_float
            = (double) timestamp->tv_sec + ((double) timestamp->tv_nsec * 1e-9);
    }

    GglMap args = GGL_MAP(
        ggl_kv(GGL_STR("keyPath"), ggl_obj_list(path_vec.list)),
        ggl_kv(GGL_STR("timestamp"), ggl_obj_f64(timestamp_float)),
        ggl_kv(GGL_STR("valueToMerge"), value_to_merge)
    );

    return ggipc_call(
        GGL_STR("aws.greengrass#UpdateConfiguration"),
        GGL_STR("aws.greengrass#UpdateConfigurationRequest"),
        args,
        NULL,
        &error_handler,
        NULL
    );
}
