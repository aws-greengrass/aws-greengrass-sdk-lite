// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <errno.h>
#include <gg/error.h>
#include <gg/log.h>
#include <gg/utils.h>
#include <time.h>
#include <stdint.h>

static GgError sleep_timespec(struct timespec time) {
    struct timespec remain = time;

    while (nanosleep(&remain, &remain) != 0) {
        if (errno != EINTR) {
            GG_LOGE("nanosleep failed: %d.", errno);
            // TODO: panic instead of returning error.
            return GG_ERR_FAILURE;
        }
    }

    return GG_ERR_OK;
}

GgError gg_sleep(int64_t seconds) {
    struct timespec time = { .tv_sec = seconds };
    return sleep_timespec(time);
}

GgError gg_sleep_ms(int64_t ms) {
    struct timespec time = { .tv_sec = ms / 1000,
                             .tv_nsec = (int32_t) ((ms % 1000) * 1000 * 1000) };
    return sleep_timespec(time);
}
