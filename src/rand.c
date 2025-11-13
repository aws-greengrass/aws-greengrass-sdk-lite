// aws-greengrass-lite - AWS IoT Greengrass runtime for constrained devices
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "gg/rand.h"
#include <errno.h>
#include <fcntl.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/file.h>
#include <gg/log.h>
#include <stdlib.h>

static int random_fd;

__attribute__((constructor)) static void init_urandom_fd(void) {
    random_fd = open("/dev/random", O_RDONLY | O_CLOEXEC);
    if (random_fd == -1) {
        GG_LOGE("Failed to open /dev/random: %d.", errno);
        _Exit(1);
    }
}

void gg_rand_fill(GgBuffer buf) {
    GgError ret = gg_file_read_exact(random_fd, buf);
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to read from /dev/random.");
        _Exit(1);
    }
}
