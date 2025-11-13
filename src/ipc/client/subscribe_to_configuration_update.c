// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/flags.h>
#include <gg/ipc/client.h>
#include <gg/ipc/client_raw.h>
#include <gg/list.h>
#include <gg/log.h>
#include <gg/map.h>
#include <gg/object.h>
#include <gg/vector.h>
#include <stddef.h>

static GgError subscribe_to_configuration_update_resp_handler(
    void *ctx,
    void *aux_ctx,
    GgIpcSubscriptionHandle handle,
    GgBuffer service_model_type,
    GgMap data
) {
    GgIpcSubscribeToConfigurationUpdateCallback *callback = ctx;

    if (!gg_buffer_eq(
            service_model_type,
            GG_STR("aws.greengrass#ConfigurationUpdateEvents")
        )) {
        GG_LOGE("Unexpected service-model-type received.");
        return GG_ERR_INVALID;
    }

    GgObject *config_update_event_obj;
    GgError ret = gg_map_validate(
        data,
        GG_MAP_SCHEMA(
            { GG_STR("configurationUpdateEvent"),
              GG_REQUIRED,
              GG_TYPE_MAP,
              &config_update_event_obj },
        )
    );
    if (ret != GG_ERR_OK) {
        GG_LOGE("Received invalid configuration update response.");
        return GG_ERR_INVALID;
    }

    GgObject *component_name_obj;
    GgObject *key_path_obj;
    ret = gg_map_validate(
        gg_obj_into_map(*config_update_event_obj),
        GG_MAP_SCHEMA(
            { GG_STR("componentName"),
              GG_REQUIRED,
              GG_TYPE_BUF,
              &component_name_obj },
            { GG_STR("keyPath"), GG_REQUIRED, GG_TYPE_LIST, &key_path_obj },
        )
    );
    if (ret != GG_ERR_OK) {
        GG_LOGE("Received invalid configuration update event.");
        return GG_ERR_INVALID;
    }

    GgBuffer component_name = gg_obj_into_buf(*component_name_obj);
    GgList key_path = gg_obj_into_list(*key_path_obj);

    ret = gg_list_type_check(key_path, GG_TYPE_BUF);
    if (ret != GG_ERR_OK) {
        GG_LOGE("Key path must contain only buffers.");
        return GG_ERR_INVALID;
    }

    callback(aux_ctx, component_name, key_path, handle);

    return GG_ERR_OK;
}

static GgError error_handler(void *ctx, GgBuffer error_code, GgBuffer message) {
    (void) ctx;

    GG_LOGE(
        "Received SubscribeToConfigurationUpdate error %.*s: %.*s.",
        (int) error_code.len,
        error_code.data,
        (int) message.len,
        message.data
    );

    if (gg_buffer_eq(error_code, GG_STR("ServiceError"))) {
        return GG_ERR_INVALID;
    }
    if (gg_buffer_eq(error_code, GG_STR("ResourceNotFoundError"))) {
        return GG_ERR_NOENTRY;
    }
    return GG_ERR_FAILURE;
}

GgError ggipc_subscribe_to_configuration_update(
    const GgBuffer *component_name,
    GgBufList key_path,
    GgIpcSubscribeToConfigurationUpdateCallback *callback,
    void *ctx,
    GgIpcSubscriptionHandle *handle
) {
    GgKVVec args = GG_KV_VEC((GgKV[2]) { 0 });

    if (component_name != NULL) {
        (void) gg_kv_vec_push(
            &args, gg_kv(GG_STR("componentName"), gg_obj_buf(*component_name))
        );
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
    (void) gg_kv_vec_push(
        &args, gg_kv(GG_STR("keyPath"), gg_obj_list(path_vec.list))
    );

    return ggipc_subscribe(
        GG_STR("aws.greengrass#SubscribeToConfigurationUpdate"),
        GG_STR("aws.greengrass#SubscribeToConfigurationUpdateRequest"),
        args.map,
        NULL,
        &error_handler,
        NULL,
        &subscribe_to_configuration_update_resp_handler,
        callback,
        ctx,
        handle
    );
}
