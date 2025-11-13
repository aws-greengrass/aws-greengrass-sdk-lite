// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/ipc/client.h>
#include <gg/ipc/client_raw.h>
#include <gg/log.h>
#include <gg/map.h>
#include <gg/object.h>
#include <gg/vector.h>
#include <time.h>

struct timespec;

static GgError error_handler(void *ctx, GgBuffer error_code, GgBuffer message) {
    (void) ctx;

    GG_LOGE(
        "Received UpdateConfig error %.*s: %.*s.",
        (int) error_code.len,
        error_code.data,
        (int) message.len,
        message.data
    );

    return GG_ERR_FAILURE;
}

GgError ggipc_update_config(
    GgBufList key_path,
    const struct timespec *timestamp,
    GgObject value_to_merge
) {
    if ((timestamp != NULL)
        && ((timestamp->tv_sec < 0) || (timestamp->tv_nsec < 0))) {
        GG_LOGE("Timestamp is negative.");
        return GG_ERR_UNSUPPORTED;
    }

    GgKV pair_to_merge;
    GgObjectType type = gg_obj_type(value_to_merge);
    if (type != GG_TYPE_MAP) {
        // For v2 compatibility, map the final key in the path to the
        // value_to_merge
        if (key_path.len == 0) {
            GG_LOGE("Root configuration object must be a map.");
            return GG_ERR_INVALID;
        }
        pair_to_merge = gg_kv(key_path.bufs[key_path.len - 1], value_to_merge);
        value_to_merge = gg_obj_map((GgMap) {
            .pairs = &pair_to_merge,
            .len = 1,
        });
        key_path.len -= 1;
    }

    GgObjVec path_vec = GG_OBJ_VEC((GgObject[GG_MAX_OBJECT_DEPTH - 1]) { 0 });
    GgError ret = GG_ERR_OK;
    for (size_t i = 0; i < key_path.len; i++) {
        gg_obj_vec_chain_push(&ret, &path_vec, gg_obj_buf(key_path.bufs[i]));
    }
    if (ret != GG_ERR_OK) {
        GG_LOGE("Key path too long.");
        return GG_ERR_NOMEM;
    }

    double timestamp_float = 0.0;
    if (timestamp != NULL) {
        timestamp_float
            = (double) timestamp->tv_sec + ((double) timestamp->tv_nsec * 1e-9);
    }

    GgMap args = GG_MAP(
        gg_kv(GG_STR("keyPath"), gg_obj_list(path_vec.list)),
        gg_kv(GG_STR("timestamp"), gg_obj_f64(timestamp_float)),
        gg_kv(GG_STR("valueToMerge"), value_to_merge)
    );

    return ggipc_call(
        GG_STR("aws.greengrass#UpdateConfiguration"),
        GG_STR("aws.greengrass#UpdateConfigurationRequest"),
        args,
        NULL,
        &error_handler,
        NULL
    );
}
