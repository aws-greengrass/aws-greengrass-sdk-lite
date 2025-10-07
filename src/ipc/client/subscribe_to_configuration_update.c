// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/flags.h>
#include <ggl/ipc/client.h>
#include <ggl/ipc/client_raw.h>
#include <ggl/log.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <ggl/vector.h>
#include <stddef.h>

static GglError subscribe_to_configuration_update_resp_handler(
    void *ctx,
    GgIpcSubscriptionHandle handle,
    GglBuffer service_model_type,
    GglMap data
) {
    GgIpcSubscribeToConfigurationUpdateCallback *handler = ctx;

    if (!ggl_buffer_eq(
            service_model_type,
            GGL_STR("aws.greengrass#ConfigurationUpdateEvents")
        )) {
        GGL_LOGE("Unexpected service-model-type received.");
        return GGL_ERR_INVALID;
    }

    GglObject *config_update_event_obj;
    GglError ret = ggl_map_validate(
        data,
        GGL_MAP_SCHEMA(
            { GGL_STR("configurationUpdateEvent"),
              GGL_REQUIRED,
              GGL_TYPE_MAP,
              &config_update_event_obj },
        )
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Received invalid configuration update response.");
        return GGL_ERR_INVALID;
    }

    GglObject *component_name_obj;
    GglObject *key_path_obj;
    ret = ggl_map_validate(
        ggl_obj_into_map(*config_update_event_obj),
        GGL_MAP_SCHEMA(
            { GGL_STR("componentName"),
              GGL_REQUIRED,
              GGL_TYPE_BUF,
              &component_name_obj },
            { GGL_STR("keyPath"), GGL_REQUIRED, GGL_TYPE_LIST, &key_path_obj },
        )
    );
    if (ret != GGL_ERR_OK) {
        GGL_LOGE("Received invalid configuration update event.");
        return GGL_ERR_INVALID;
    }

    GglBuffer component_name = ggl_obj_into_buf(*component_name_obj);
    GglList key_path = ggl_obj_into_list(*key_path_obj);

    if (handler != NULL) {
        handler(component_name, key_path, handle);
    }

    return GGL_ERR_OK;
}

static GglError error_handler(
    void *ctx, GglBuffer error_code, GglBuffer message
) {
    (void) ctx;

    GGL_LOGE(
        "Received SubscribeToConfigurationUpdate error %.*s: %.*s.",
        (int) error_code.len,
        error_code.data,
        (int) message.len,
        message.data
    );

    if (ggl_buffer_eq(error_code, GGL_STR("ServiceError"))) {
        return GGL_ERR_INVALID;
    }
    if (ggl_buffer_eq(error_code, GGL_STR("ResourceNotFoundError"))) {
        return GGL_ERR_NOENTRY;
    }
    return GGL_ERR_FAILURE;
}

GglError ggipc_subscribe_to_configuration_update(
    const GglBuffer *component_name,
    GglBufList key_path,
    GgIpcSubscribeToConfigurationUpdateCallback *callback,
    GgIpcSubscriptionHandle *handle
) {
    GglKVVec args = GGL_KV_VEC((GglKV[2]) { 0 });

    if (component_name != NULL) {
        (void) ggl_kv_vec_push(
            &args,
            ggl_kv(GGL_STR("componentName"), ggl_obj_buf(*component_name))
        );
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
    (void) ggl_kv_vec_push(
        &args, ggl_kv(GGL_STR("keyPath"), ggl_obj_list(path_vec.list))
    );

    return ggipc_subscribe(
        GGL_STR("aws.greengrass#SubscribeToConfigurationUpdate"),
        GGL_STR("aws.greengrass#SubscribeToConfigurationUpdateRequest"),
        args.map,
        NULL,
        &error_handler,
        NULL,
        &subscribe_to_configuration_update_resp_handler,
        callback,
        handle
    );
}
