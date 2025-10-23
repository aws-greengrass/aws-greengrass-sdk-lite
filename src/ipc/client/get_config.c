// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/arena.h>
#include <ggl/attr.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/flags.h>
#include <ggl/ipc/client.h>
#include <ggl/ipc/client_raw.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <ggl/vector.h>
#include <string.h>

static GglError error_handler(
    void *ctx, GglBuffer error_code, GglBuffer message
) {
    (void) ctx;

    if (ggl_buffer_eq(error_code, GGL_STR("ResourceNotFoundError"))) {
        GGL_LOGW(
            "Requested configuration could not be found: %.*s",
            (int) message.len,
            message.data
        );
        return GGL_ERR_NOENTRY;
    }

    GGL_LOGE(
        "Received PublishToTopic error %.*s: %.*s.",
        (int) error_code.len,
        error_code.data,
        (int) message.len,
        message.data
    );

    return GGL_ERR_FAILURE;
}

NONNULL(3)
static GglError ggipc_get_config_common(
    GglBufList key_path,
    const GglBuffer *component_name,
    GgIpcResultCallback *result_callback,
    void *result_ctx
) {
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

    return ggipc_call(
        GGL_STR("aws.greengrass#GetConfiguration"),
        GGL_STR("aws.greengrass#GetConfigurationRequest"),
        args.map,
        result_callback,
        &error_handler,
        result_ctx
    );
}

static GglError get_resp_value(
    GglMap resp, GglObject **value, GglBuffer *final_key
) {
    GglError ret = ggl_map_validate(
        resp,
        GGL_MAP_SCHEMA({ GGL_STR("value"), GGL_REQUIRED, GGL_TYPE_MAP, value })
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Failed validating server response.");
        return GGL_ERR_INVALID;
    }

    GglMap map = ggl_obj_into_map(**value);

    // Maps IPC response according to classic behavior
    if ((final_key != NULL) && (map.len == 1)
        && (ggl_buffer_eq(ggl_kv_key(map.pairs[0]), *final_key))
        && (ggl_obj_type(*ggl_kv_val(&map.pairs[0])) != GGL_TYPE_MAP)) {
        *value = ggl_kv_val(&map.pairs[0]);
    }

    return GGL_ERR_OK;
}

typedef struct {
    GglObject *value;
    GglArena *alloc;
    GglBuffer *final_key;
} CopyObjectCtx;

static GglError copy_config_obj(void *ctx, GglMap result) {
    CopyObjectCtx *copy_ctx = ctx;

    GglObject *value;
    GglError ret = get_resp_value(result, &value, copy_ctx->final_key);
    if (ret != GGL_ERR_OK) {
        return GGL_ERR_INVALID;
    }

    if (copy_ctx->value != NULL) {
        ret = ggl_arena_claim_obj(value, copy_ctx->alloc);
        if (ret != GGL_ERR_OK) {
            GGL_LOGE("Insufficent memory provided for response.");
            return ret;
        }

        *copy_ctx->value = *value;
    }

    return GGL_ERR_OK;
}

GglError ggipc_get_config(
    GglBufList key_path,
    const GglBuffer *component_name,
    GglArena *alloc,
    GglObject *value
) {
    if (value != NULL) {
        *value = GGL_OBJ_NULL;
    }

    CopyObjectCtx response_ctx
        = { .value = value,
            .alloc = alloc,
            .final_key
            = (key_path.len == 0) ? NULL : &key_path.bufs[key_path.len - 1] };
    return ggipc_get_config_common(
        key_path, component_name, &copy_config_obj, &response_ctx
    );
}

typedef struct {
    GglBuffer *value;
    GglBuffer *final_key;
} CopyBufferCtx;

static GglError copy_config_buf(void *ctx, GglMap result) {
    CopyBufferCtx *resp_ctx = ctx;

    GglObject *value;
    GglError ret = get_resp_value(result, &value, resp_ctx->final_key);
    if (ret != GGL_ERR_OK) {
        return GGL_ERR_INVALID;
    }

    if (ggl_obj_type(*value) != GGL_TYPE_BUF) {
        GGL_LOGE(
            "Config value is not a string. Type: %d", ggl_obj_type(*value)
        );
        return GGL_ERR_FAILURE;
    }

    if (resp_ctx->value != NULL) {
        GglBuffer value_buf = ggl_obj_into_buf(*value);

        GglArena alloc = ggl_arena_init(*resp_ctx->value);
        ret = ggl_arena_claim_buf(&value_buf, &alloc);
        if (ret != GGL_ERR_OK) {
            GGL_LOGE("Insufficent memory provided for response.");
            return ret;
        }

        *resp_ctx->value = value_buf;
    }

    return GGL_ERR_OK;
}

GglError ggipc_get_config_str(
    GglBufList key_path, const GglBuffer *component_name, GglBuffer *value
) {
    CopyBufferCtx copy_ctx
        = { .value = value,
            .final_key
            = (key_path.len == 0) ? NULL : &key_path.bufs[key_path.len - 1] };

    GglError ret = ggipc_get_config_common(
        key_path, component_name, &copy_config_buf, &copy_ctx
    );
    if ((ret != GGL_ERR_OK) && (value != NULL)) {
        *value = GGL_STR("");
    }
    return ret;
}
