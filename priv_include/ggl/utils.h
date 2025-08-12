// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_UTILS_H
#define GGL_UTILS_H

//! Misc utilities

#include <ggl/attr.h>
#include <ggl/error.h>
#include <stdint.h>

/// Sleep for given duration in seconds.
VISIBILITY(hidden)
GglError ggl_sleep(int64_t seconds);

/// Sleep for given duration in milliseconds.
VISIBILITY(hidden)
GglError ggl_sleep_ms(int64_t ms);

#endif
