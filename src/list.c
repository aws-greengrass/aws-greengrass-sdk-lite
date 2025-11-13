// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/error.h>
#include <gg/list.h>
#include <gg/log.h>
#include <gg/object.h>

GgError gg_list_type_check(GgList list, GgObjectType type) {
    GG_LIST_FOREACH(elem, list) {
        if (gg_obj_type(*elem) != type) {
            GG_LOGE("List element is of invalid type.");
            return GG_ERR_PARSE;
        }
    }
    return GG_ERR_OK;
}
