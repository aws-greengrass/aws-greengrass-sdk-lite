// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_EVENTSTREAM_CRC_H
#define GGL_EVENTSTREAM_CRC_H

#include <ggl/buffer.h>
#include <stdint.h>

/// Update a running crc with the given bytes.
/// Initial value of `crc` should be 0.
uint32_t ggl_update_crc(uint32_t crc, GglBuffer buf);

#endif
