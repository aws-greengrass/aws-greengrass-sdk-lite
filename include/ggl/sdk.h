// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_SDK_H
#define GGL_SDK_H

#include <ggl/attr.h>

/// Initializes the sdk, including starting necessary threads.
/// Unused portions of sdk may not be initialized.
/// Exits on error.
VISIBILITY(default)
void ggl_sdk_init(void);

#endif
