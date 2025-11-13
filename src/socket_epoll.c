// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <errno.h>
#include <gg/error.h>
#include <gg/log.h>
#include <gg/socket_epoll.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

GgError gg_socket_epoll_create(int *epoll_fd) {
    int fd = epoll_create1(EPOLL_CLOEXEC);
    if (fd == -1) {
        int err = errno;
        GG_LOGE("Failed to create epoll fd: %d.", err);
        return GG_ERR_FAILURE;
    }
    *epoll_fd = fd;
    return GG_ERR_OK;
}

GgError gg_socket_epoll_add(int epoll_fd, int target_fd, uint64_t data) {
    assert(epoll_fd >= 0);
    assert(target_fd >= 0);

    struct epoll_event event = { .events = EPOLLIN, .data = { .u64 = data } };

    int err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, target_fd, &event);
    if (err == -1) {
        err = errno;
        GG_LOGE("Failed to add watch for %d: %d.", target_fd, err);
        return GG_ERR_FAILURE;
    }
    return GG_ERR_OK;
}

GgError gg_socket_epoll_run(
    int epoll_fd, GgError (*fd_ready)(void *ctx, uint64_t data), void *ctx
) {
    assert(epoll_fd >= 0);
    assert(fd_ready != NULL);

    int32_t tid = gettid();

    GG_LOGD("Entering epoll loop on thread %d.", tid);

    struct epoll_event events[10] = { 0 };

    while (true) {
        int ready = epoll_wait(
            epoll_fd, events, sizeof(events) / sizeof(*events), -1
        );

        if (ready == -1) {
            if (errno == EINTR) {
                GG_LOGT("epoll_wait interrupted on thread %d.", tid);
                continue;
            }
            GG_LOGE("Failed to wait on epoll on thread %d: %d.", tid, errno);
            return GG_ERR_FAILURE;
        }

        for (int i = 0; i < ready; i++) {
            GG_LOGD("Calling epoll callback on thread %d.", tid);
            GgError ret = fd_ready(ctx, events[i].data.u64);
            if (ret != GG_ERR_OK) {
                return ret;
            }
        }
    }

    return GG_ERR_FAILURE;
}
