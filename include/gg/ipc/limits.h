// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_IPC_LIMITS_H
#define GG_IPC_LIMITS_H

#define GG_IPC_SVCUID_STR_LEN (16)

/// Maximum size of eventstream packet.
/// Can be configured with `-D GG_IPC_MAX_MSG_LEN=<N>`.
#ifndef GG_IPC_MAX_MSG_LEN
#define GG_IPC_MAX_MSG_LEN 10000
#endif

#endif
