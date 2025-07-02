// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_LIMITS_H
#define GGL_IPC_LIMITS_H

#define GGL_IPC_PAYLOAD_MAX_SUBOBJECTS (50)
#define GGL_IPC_SVCUID_STR_LEN (16)

/// Maximum size of eventstream packet.
/// Can be configured with `-D GGL_IPC_MAX_MSG_LEN=<N>`.
#ifndef GGL_IPC_MAX_MSG_LEN
#define GGL_IPC_MAX_MSG_LEN 10000
#endif

#endif
