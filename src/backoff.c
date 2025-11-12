// aws-greengrass-lite - AWS IoT Greengrass runtime for constrained devices
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <ggl/backoff.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/log.h>
#include <ggl/rand.h>
#include <ggl/utils.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static uint64_t backoff_get_rand(void) {
    uint64_t val;
    ggl_rand_fill((GglBuffer) { .data = (uint8_t *) &val, .len = sizeof(val) });
    return val;
}

static void backoff_sleep(uint32_t ms) {
    GglError sleep_err = ggl_sleep_ms(ms);
    if (sleep_err != GGL_ERR_OK) {
        GGL_LOGE("Fatal error: unexpected sleep error during backoff.");
        _Exit(1);
    }
}

GglError ggl_backoff(
    uint32_t base_ms,
    uint32_t max_ms,
    uint32_t max_attempts,
    GglError (*fn)(void *ctx),
    void *ctx
) {
    if (fn == NULL) {
        assert(false);
        return GGL_ERR_UNSUPPORTED;
    }
    if (base_ms == 0) {
        assert(false);
        return GGL_ERR_UNSUPPORTED;
    }

    uint32_t current_max_ms = base_ms;
    uint32_t attempts = 0;
    GglError ret = GGL_ERR_FAILURE;

    while (true) {
        ret = fn(ctx);
        if (ret == GGL_ERR_OK) {
            return GGL_ERR_OK;
        }

        if (max_attempts != 0) {
            attempts += 1;
            if (attempts == max_attempts) {
                return ret;
            }
        }

        // Approximately uniform; final wraparound is negligible as rand is
        // 64-bit and ms is 32-bit
        backoff_sleep((uint32_t) (backoff_get_rand() % current_max_ms));

        if (current_max_ms <= (max_ms / 2)) {
            current_max_ms *= 2;
        } else {
            current_max_ms = max_ms;
        }
    }
}
