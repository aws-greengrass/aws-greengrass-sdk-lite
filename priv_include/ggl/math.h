// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_MATH_H
#define GGL_MATH_H

//! Math utilities

#include <stdint.h>

/// Absolute value, avoiding undefined behavior.
/// i.e. avoiding -INT64_MIN for twos-compliment)
uint64_t ggl_abs(int64_t i64);

#endif
