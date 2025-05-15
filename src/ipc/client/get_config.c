// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/arena.h>
#include <ggl/buffer.h>
#include <ggl/constants.h>
#include <ggl/error.h>
#include <ggl/flags.h>
#include <ggl/ipc/client.h>
#include <ggl/ipc/client_raw.h>
#include <ggl/ipc/error.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <ggl/vector.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>

GglError ggipc_get_config_str(
    int conn, GglBufList key_path, GglBuffer *component_name, GglBuffer *value
) {
    GglObjVec path_vec = GGL_OBJ_VEC((GglObject[GGL_MAX_OBJECT_DEPTH]) { 0 });
    GglError ret = GGL_ERR_OK;
    for (size_t i = 0; i < key_path.len; i++) {
        ggl_obj_vec_chain_push(&ret, &path_vec, ggl_obj_buf(key_path.bufs[i]));
    }
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Key path too long.");
        return GGL_ERR_NOMEM;
    }

    GglKVVec args = GGL_KV_VEC((GglKV[2]) { 0 });
    (void) ggl_kv_vec_push(
        &args, ggl_kv(GGL_STR("keyPath"), ggl_obj_list(path_vec.list))
    );
    if (component_name != NULL) {
        (void) ggl_kv_vec_push(
            &args,
            ggl_kv(GGL_STR("componentName"), ggl_obj_buf(*component_name))
        );
    }

    static uint8_t resp_mem[sizeof(GglKV) + sizeof("value") + PATH_MAX];
    GglArena alloc = ggl_arena_init(GGL_BUF(resp_mem));
    GglObject resp = { 0 };
    GglIpcError remote_error = GGL_IPC_ERROR_DEFAULT;
    ret = ggipc_call(
        conn,
        GGL_STR("aws.greengrass#GetConfiguration"),
        GGL_STR("aws.greengrass#GetConfigurationRequest"),
        args.map,
        &alloc,
        &resp,
        &remote_error
    );
    if (ret == GGL_ERR_REMOTE) {
        if (remote_error.error_code == GGL_IPC_ERR_RESOURCE_NOT_FOUND) {
            GGL_LOGE(
                "Requested configuration could not be found: %.*s",
                (int) remote_error.message.len,
                remote_error.message.data
            );
            return GGL_ERR_NOENTRY;
        }

        GGL_LOGE("Server error.");
        return GGL_ERR_FAILURE;
    }

    if (ret != GGL_ERR_OK) {
        return ret;
    }

    if (ggl_obj_type(resp) != GGL_TYPE_MAP) {
        GGL_LOGE("Config value is not a map.");
        return GGL_ERR_FAILURE;
    }

    GglObject *resp_value_obj;
    ret = ggl_map_validate(
        ggl_obj_into_map(resp),
        GGL_MAP_SCHEMA(
            { GGL_STR("value"), GGL_REQUIRED, GGL_TYPE_BUF, &resp_value_obj }
        )
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed validating server response.");
        return GGL_ERR_INVALID;
    }
    GglBuffer resp_value = ggl_obj_into_buf(*resp_value_obj);

    if (value != NULL) {
        GglArena ret_alloc = ggl_arena_init(*value);
        ret = ggl_arena_claim_buf(&resp_value, &ret_alloc);
        if (ret != GGL_ERR_OK) {
            GGL_LOGE("Insufficent memory provided for response.");
            return ret;
        }

        *value = resp_value;
    }
    return GGL_ERR_OK;
}

GglError ggipc_get_config_obj(
    int conn,
    GglBufList key_path,
    GglBuffer *component_name,
    GglArena *alloc,
    GglObject *value
) {
    GglObjVec path_vec = GGL_OBJ_VEC((GglObject[GGL_MAX_OBJECT_DEPTH]) { 0 });
    GglError ret = GGL_ERR_OK;
    for (size_t i = 0; i < key_path.len; i++) {
        ggl_obj_vec_chain_push(&ret, &path_vec, ggl_obj_buf(key_path.bufs[i]));
    }
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Key path too long.");
        return GGL_ERR_NOMEM;
    }

    GglKVVec args = GGL_KV_VEC((GglKV[2]) { 0 });
    (void) ggl_kv_vec_push(
        &args, ggl_kv(GGL_STR("keyPath"), ggl_obj_list(path_vec.list))
    );
    if (component_name != NULL) {
        (void) ggl_kv_vec_push(
            &args,
            ggl_kv(GGL_STR("componentName"), ggl_obj_buf(*component_name))
        );
    }

    static uint8_t resp_mem[sizeof(GglKV) + sizeof("value") + PATH_MAX];
    GglArena resp_alloc = ggl_arena_init(GGL_BUF(resp_mem));
    GglObject resp = { 0 };
    GglIpcError remote_error = GGL_IPC_ERROR_DEFAULT;
    ret = ggipc_call(
        conn,
        GGL_STR("aws.greengrass#GetConfiguration"),
        GGL_STR("aws.greengrass#GetConfigurationRequest"),
        args.map,
        &resp_alloc,
        &resp,
        &remote_error
    );
    if (ret == GGL_ERR_REMOTE) {
        if (remote_error.error_code == GGL_IPC_ERR_RESOURCE_NOT_FOUND) {
            GGL_LOGE(
                "Requested configuration could not be found: %.*s",
                (int) remote_error.message.len,
                remote_error.message.data
            );
            return GGL_ERR_NOENTRY;
        }

        GGL_LOGE("Server error.");
        return GGL_ERR_FAILURE;
    }

    if (ret != GGL_ERR_OK) {
        return ret;
    }

    if (ggl_obj_type(resp) != GGL_TYPE_MAP) {
        GGL_LOGE("Config value is not a map.");
        return GGL_ERR_FAILURE;
    }

    GglObject *resp_value;
    ret = ggl_map_validate(
        ggl_obj_into_map(resp),
        GGL_MAP_SCHEMA(
            { GGL_STR("value"), GGL_REQUIRED, GGL_TYPE_NULL, &resp_value }
        )
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed validating server response.");
        return GGL_ERR_INVALID;
    }

    if (value != NULL) {
        ret = ggl_arena_claim_obj(resp_value, alloc);
        if (ret != GGL_ERR_OK) {
            GGL_LOGE("Insufficent memory provided for response.");
            return ret;
        }

        *value = *resp_value;
    }
    return GGL_ERR_OK;
}
