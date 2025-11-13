// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_UTILS_H
#define GG_UTILS_H

//! Misc utilities

#include <gg/attr.h>
#include <gg/error.h>
#include <stdint.h>

/// Sleep for given duration in seconds.
VISIBILITY(hidden)
GgError gg_sleep(int64_t seconds);

/// Sleep for given duration in milliseconds.
VISIBILITY(hidden)
GgError gg_sleep_ms(int64_t ms);

#endif
