// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/arena.h>
#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/flags.h>
#include <gg/ipc/client.h>
#include <gg/ipc/client_raw.h>
#include <gg/log.h>
#include <gg/map.h>
#include <gg/object.h>
#include <gg/vector.h>
#include <string.h>

static GgError error_handler(void *ctx, GgBuffer error_code, GgBuffer message) {
    (void) ctx;

    if (gg_buffer_eq(error_code, GG_STR("ResourceNotFoundError"))) {
        GG_LOGW(
            "Requested configuration could not be found: %.*s",
            (int) message.len,
            message.data
        );
        return GG_ERR_NOENTRY;
    }

    GG_LOGE(
        "Received PublishToTopic error %.*s: %.*s.",
        (int) error_code.len,
        error_code.data,
        (int) message.len,
        message.data
    );

    return GG_ERR_FAILURE;
}

NONNULL(3)
static GgError ggipc_get_config_common(
    GgBufList key_path,
    const GgBuffer *component_name,
    GgIpcResultCallback *result_callback,
    void *result_ctx
) {
    GgObjVec path_vec = GG_OBJ_VEC((GgObject[GG_MAX_OBJECT_DEPTH - 1]) { 0 });
    GgError ret = GG_ERR_OK;
    for (size_t i = 0; i < key_path.len; i++) {
        gg_obj_vec_chain_push(&ret, &path_vec, gg_obj_buf(key_path.bufs[i]));
    }
    if (ret != GG_ERR_OK) {
        GG_LOGE("Key path too long.");
        return GG_ERR_NOMEM;
    }

    GgKVVec args = GG_KV_VEC((GgKV[2]) { 0 });
    (void) gg_kv_vec_push(
        &args, gg_kv(GG_STR("keyPath"), gg_obj_list(path_vec.list))
    );
    if (component_name != NULL) {
        (void) gg_kv_vec_push(
            &args, gg_kv(GG_STR("componentName"), gg_obj_buf(*component_name))
        );
    }

    return ggipc_call(
        GG_STR("aws.greengrass#GetConfiguration"),
        GG_STR("aws.greengrass#GetConfigurationRequest"),
        args.map,
        result_callback,
        &error_handler,
        result_ctx
    );
}

static GgError get_resp_value(
    GgMap resp, GgObject **value, GgBuffer *final_key
) {
    GgError ret = gg_map_validate(
        resp,
        GG_MAP_SCHEMA({ GG_STR("value"), GG_REQUIRED, GG_TYPE_MAP, value })
    );
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed validating server response.");
        return GG_ERR_INVALID;
    }

    GgMap map = gg_obj_into_map(**value);

    // Maps IPC response according to classic behavior
    if ((final_key != NULL) && (map.len == 1)
        && (gg_buffer_eq(gg_kv_key(map.pairs[0]), *final_key))
        && (gg_obj_type(*gg_kv_val(&map.pairs[0])) != GG_TYPE_MAP)) {
        *value = gg_kv_val(&map.pairs[0]);
    }

    return GG_ERR_OK;
}

typedef struct {
    GgObject *value;
    GgArena *alloc;
    GgBuffer *final_key;
} CopyObjectCtx;

static GgError copy_config_obj(void *ctx, GgMap result) {
    CopyObjectCtx *copy_ctx = ctx;

    GgObject *value;
    GgError ret = get_resp_value(result, &value, copy_ctx->final_key);
    if (ret != GG_ERR_OK) {
        return GG_ERR_INVALID;
    }

    if (copy_ctx->value != NULL) {
        ret = gg_arena_claim_obj(value, copy_ctx->alloc);
        if (ret != GG_ERR_OK) {
            GG_LOGE("Insufficent memory provided for response.");
            return ret;
        }

        *copy_ctx->value = *value;
    }

    return GG_ERR_OK;
}

GgError ggipc_get_config(
    GgBufList key_path,
    const GgBuffer *component_name,
    GgArena *alloc,
    GgObject *value
) {
    if (value != NULL) {
        *value = GG_OBJ_NULL;
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
    GgBuffer *value;
    GgBuffer *final_key;
} CopyBufferCtx;

static GgError copy_config_buf(void *ctx, GgMap result) {
    CopyBufferCtx *resp_ctx = ctx;

    GgObject *value;
    GgError ret = get_resp_value(result, &value, resp_ctx->final_key);
    if (ret != GG_ERR_OK) {
        return GG_ERR_INVALID;
    }

    if (gg_obj_type(*value) != GG_TYPE_BUF) {
        GG_LOGE("Config value is not a string. Type: %d", gg_obj_type(*value));
        return GG_ERR_FAILURE;
    }

    if (resp_ctx->value != NULL) {
        GgBuffer value_buf = gg_obj_into_buf(*value);

        GgArena alloc = gg_arena_init(*resp_ctx->value);
        ret = gg_arena_claim_buf(&value_buf, &alloc);
        if (ret != GG_ERR_OK) {
            GG_LOGE("Insufficent memory provided for response.");
            return ret;
        }

        *resp_ctx->value = value_buf;
    }

    return GG_ERR_OK;
}

GgError ggipc_get_config_str(
    GgBufList key_path, const GgBuffer *component_name, GgBuffer *value
) {
    CopyBufferCtx copy_ctx
        = { .value = value,
            .final_key
            = (key_path.len == 0) ? NULL : &key_path.bufs[key_path.len - 1] };

    GgError ret = ggipc_get_config_common(
        key_path, component_name, &copy_config_buf, &copy_ctx
    );
    if ((ret != GG_ERR_OK) && (value != NULL)) {
        *value = GG_STR("");
    }
    return ret;
}
