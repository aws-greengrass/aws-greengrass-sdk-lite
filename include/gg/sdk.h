// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_SDK_H
#define GG_SDK_H

/// Initializes the sdk, including starting necessary threads.
/// Unused portions of sdk may not be initialized.
/// Exits on error.
void gg_sdk_init(void);

#endif
