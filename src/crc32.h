// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_EVENTSTREAM_CRC_H
#define GG_EVENTSTREAM_CRC_H

#include <gg/attr.h>
#include <gg/buffer.h>
#include <stdint.h>

/// Update a running crc with the given bytes.
/// Initial value of `crc` should be 0.
VISIBILITY(hidden)
uint32_t gg_update_crc(uint32_t crc, GgBuffer buf);

#endif
