// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/error.h>
#include <gg/init.h>
#include <gg/log.h>
#include <gg/sdk.h>
#include <stdlib.h>

static GgInitEntry *init_list = NULL;

void gg_register_init_fn(GgInitEntry entry[static 1]) {
    entry->next = init_list;
    init_list = entry;
}

void gg_sdk_init(void) {
    GgInitEntry *list = init_list;
    init_list = NULL;

    while (list != NULL) {
        GgError ret = list->fn();
        if (ret != GG_ERR_OK) {
            GG_LOGE("Failed to initialize (%u).", (unsigned) ret);
            _Exit(1);
        }

        list = list->next;
    }
}
