// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/client.h>
#include <ggl/object.h>
#include <ggl/sdk.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t config_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t config_cond = PTHREAD_COND_INITIALIZER;
static bool config_updated = false;

static void config_update_handler(
    GglBuffer component_name, GglList key_path, GgIpcSubscriptionHandle handle
) {
    (void) handle;
    printf("Configuration update received:\n");
    printf(
        "  Component: %.*s\n", (int) component_name.len, component_name.data
    );
    printf("  Key path: [");

    for (size_t i = 0; i < key_path.len; i++) {
        if (i > 0) {
            printf(", ");
        }
        GglObject *obj = &key_path.items[i];
        if (ggl_obj_type(*obj) == GGL_TYPE_BUF) {
            GglBuffer key = ggl_obj_into_buf(*obj);
            printf("\"%.*s\"", (int) key.len, key.data);
        }
    }
    printf("]\n");

    pthread_mutex_lock(&config_mutex);
    config_updated = true;
    pthread_cond_signal(&config_cond);
    pthread_mutex_unlock(&config_mutex);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    ggl_sdk_init();

    GglError ret = ggipc_connect();
    if (ret != GGL_ERR_OK) {
        fprintf(stderr, "Failed to connect to IPC: %d\n", ret);
        return EXIT_FAILURE;
    }

    printf("Connected to Greengrass IPC\n");

    // A key with 0 elements means subscribe to all config updates for the
    // component
    GglBufList key_path = GGL_BUF_LIST(GGL_STR("test_str"));
    ret = ggipc_subscribe_to_configuration_update(
        NULL, key_path, config_update_handler, NULL
    );

    if (ret != GGL_ERR_OK) {
        fprintf(
            stderr, "Failed to subscribe to configuration updates: %d\n", ret
        );
        return EXIT_FAILURE;
    }

    printf("Subscribed to configuration updates. Waiting for updates...\n");

    uint8_t prev_mem[500] = { 0 };
    GglBuffer prev_value = GGL_BUF(prev_mem);

    while (1) {
        pthread_mutex_lock(&config_mutex);
        while (!config_updated) {
            pthread_cond_wait(&config_cond, &config_mutex);
        }
        config_updated = false;
        pthread_mutex_unlock(&config_mutex);

        uint8_t config_mem[500];
        GglBuffer config = GGL_BUF(config_mem);
        ret = ggipc_get_config_str(key_path, NULL, &config);
        if (ret != GGL_ERR_OK) {
            fprintf(stderr, "Failed to get configuration: %d\n", ret);
            continue;
        }

        if (!ggl_buffer_eq(config, prev_value)) {
            printf(
                "Updated configuration value: %.*s\n",
                (int) config.len,
                config.data
            );
            memcpy(prev_value.data, config.data, config.len);
            prev_value.len = config.len;
        }
    }
}
