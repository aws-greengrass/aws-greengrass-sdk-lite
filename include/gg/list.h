// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_LIST_H
#define GG_LIST_H

//! List utilities

#include <gg/attr.h>
#include <gg/error.h>
#include <gg/object.h>

// NOLINTBEGIN(bugprone-macro-parentheses)
/// Loop over the objects in a list.
#define GG_LIST_FOREACH(name, list) \
    for (GgObject *name = (list).items; name < &(list).items[(list).len]; \
         name = &name[1])
// NOLINTEND(bugprone-macro-parentheses)

/// Check that all elements in a list are of the specified type.
/// Returns GG_ERR_OK if all elements match `type`, GG_ERR_PARSE otherwise.
PURE
GgError gg_list_type_check(GgList list, GgObjectType type);

#endif
