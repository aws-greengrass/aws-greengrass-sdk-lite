// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_SOCKET_EPOLL_H
#define GG_SOCKET_EPOLL_H

//! Epoll wrappers

#include <gg/attr.h>
#include <gg/error.h>
#include <stdint.h>

/// Create an epoll fd.
/// Caller is responsible for closing.
VISIBILITY(hidden)
GgError gg_socket_epoll_create(int *epoll_fd);

/// Add an epoll watch.
VISIBILITY(hidden)
GgError gg_socket_epoll_add(int epoll_fd, int target_fd, uint64_t data);

/// Continuously wait on epoll, calling callback when data is ready.
/// Exits only on error waiting or error from callback.
VISIBILITY(hidden)
GgError gg_socket_epoll_run(
    int epoll_fd, GgError (*fd_ready)(void *ctx, uint64_t data), void *ctx
);

#endif
