// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "crc32.h"
#include <ggl/buffer.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __has_builtin
#if __has_builtin(__builtin_rev_crc32_data8)
#define HAS_BUILTIN_CRC 1
#endif
#endif

#ifndef HAS_BUILTIN_CRC
#define HAS_BUILTIN_CRC 0
#endif

#if HAS_BUILTIN_CRC

static uint32_t crc_step(uint32_t crc, uint8_t byte) {
    return __builtin_rev_crc32_data8(crc, byte, 0x04C11DB7L)
}

#else

// CRC code adapted from rfc1952 GZIP file format specification version 4.3

/// Table of CRCs of all 8-bit messages.
/// Initialized by `make_crc_table`.
static uint32_t crc_table[256];

/// Make the table for a fast CRC.
__attribute__((constructor)) static void make_crc_table(void) {
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) {
            if (c & 1) {
                c = 0xEDB88320L ^ (c >> 1);
            } else {
                c = c >> 1;
            }
        }
        crc_table[n] = c;
    }
}

static uint32_t crc_step(uint32_t crc, uint8_t byte) {
    return crc_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
}

#endif

uint32_t ggl_update_crc(uint32_t crc, GglBuffer buf) {
    uint32_t c = ~crc;
    for (size_t n = 0; n < buf.len; n++) {
        c = crc_step(c, buf.data[n]);
    }
    return ~c;
}
