// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/error.h>
#include <ggl/init.h>
#include <ggl/log.h>
#include <ggl/sdk.h>
#include <stdlib.h>

static GglInitEntry *init_list = NULL;

void ggl_register_init_fn(GglInitEntry *entry) {
    entry->next = init_list;
    init_list = entry;
}

void ggl_sdk_init(void) {
    GglInitEntry *list = init_list;

    while (list != NULL) {
        GglError ret = list->fn();
        if (ret != GGL_ERR_OK) {
            GGL_LOGE("Failed to initialize (%u).", (unsigned) ret);
            _Exit(1);
        }

        list = list->next;
    }
}
