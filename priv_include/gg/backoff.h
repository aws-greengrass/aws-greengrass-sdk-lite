// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_BACKOFF_H
#define GG_BACKOFF_H

//! Greengrass backoff util

#include <gg/error.h>
#include <stdint.h>

/// Run `fn`, retrying with exponential backoff and full jitter
/// Pass 0 to `max_attempts` for indefinite attempts.
/// `fn` will be called until it returns GG_ERR_OK or attempts run out.
/// <https://aws.amazon.com/blogs/architecture/exponential-backoff-and-jitter/>
GgError gg_backoff(
    uint32_t base_ms,
    uint32_t max_ms,
    uint32_t max_attempts,
    GgError (*fn)(void *ctx),
    void *ctx
);

#endif
