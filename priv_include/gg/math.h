// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_MATH_H
#define GG_MATH_H

//! Math utilities

#include <gg/attr.h>
#include <stdint.h>

/// Absolute value, avoiding undefined behavior.
/// i.e. avoiding -INT64_MIN for twos-compliment)
VISIBILITY(hidden)
uint64_t gg_abs(int64_t i64);

#endif
