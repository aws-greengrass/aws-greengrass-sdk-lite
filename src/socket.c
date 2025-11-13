// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <errno.h>
#include <gg/buffer.h>
#include <gg/cleanup.h>
#include <gg/error.h>
#include <gg/file.h>
#include <gg/io.h>
#include <gg/log.h>
#include <gg/socket.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>

GgError gg_socket_read(int fd, GgBuffer buf) {
    GgError ret = gg_file_read_exact(fd, buf);
    if (ret == GG_ERR_NODATA) {
        GG_LOGD("Socket %d closed by peer.", fd);
    }
    return ret;
}

GgError gg_socket_write(int fd, GgBuffer buf) {
    return gg_file_write(fd, buf);
}

GgError gg_connect(GgBuffer path, int *fd) {
    struct sockaddr_un addr = { .sun_family = AF_UNIX, .sun_path = { 0 } };

    // TODO: Use symlinks to handle long paths
    if (path.len >= sizeof(addr.sun_path)) {
        GG_LOGE("Socket path too long.");
        return GG_ERR_FAILURE;
    }

    memcpy(addr.sun_path, path.data, path.len);

    int sockfd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (sockfd == -1) {
        GG_LOGE("Failed to create socket: %d.", errno);
        return GG_ERR_FATAL;
    }
    GG_CLEANUP_ID(sockfd_cleanup, cleanup_close, sockfd);

    if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        GG_LOGW(
            "Failed to connect to server (%.*s): %d.",
            (int) path.len,
            path.data,
            errno
        );
        return GG_ERR_FAILURE;
    }

    // To prevent deadlocking on hanged server, add a timeout
    struct timeval timeout = { .tv_sec = 5 };
    int sys_ret = setsockopt(
        sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)
    );
    if (sys_ret == -1) {
        GG_LOGE("Failed to set receive timeout on socket: %d.", errno);
        return GG_ERR_FATAL;
    }
    sys_ret = setsockopt(
        sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)
    );
    if (sys_ret == -1) {
        GG_LOGE("Failed to set send timeout on socket: %d.", errno);
        return GG_ERR_FATAL;
    }

    sockfd_cleanup = -1;
    *fd = sockfd;
    return GG_ERR_OK;
}

static GgError socket_reader_fn(void *ctx, GgBuffer *buf) {
    int *fd = ctx;
    return gg_file_read(*fd, buf);
}

GgReader gg_socket_reader(int *fd) {
    return (GgReader) { .read = socket_reader_fn, .ctx = fd };
}

static GgError socket_writer_fn(void *ctx, GgBuffer buf) {
    int *fd = ctx;
    return gg_socket_write(*fd, buf);
}

GgWriter gg_socket_writer(int *fd) {
    return (GgWriter) { .write = socket_writer_fn, .ctx = fd };
}
