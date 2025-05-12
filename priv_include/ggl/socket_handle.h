// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_SOCKET_HANDLE_H
#define GGL_SOCKET_HANDLE_H

//! Common library for managing unix sockets

#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/io.h>
#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

// Socket management using generational indices to invalidate use of dangling
// references after a socket is closed.

/// Pool of memory for client/server sockets.
/// Can be shared between multiple server/client instances.
/// `fds` and `generations` should be set to arrays of length `max_fds`.
typedef struct {
    uint16_t max_fds;
    int *fds;
    uint16_t *generations;
    GglError (*on_register)(uint32_t handle, size_t index);
    GglError (*on_release)(uint32_t handle, size_t index);
    pthread_mutex_t mtx;
} GglSocketPool;

/// Initialize the memory of a `GglSocketPool`.
/// Pointers should be set before calling this.
/// This will initialize the mutex as well as array data.
/// Safe to call before main.
void ggl_socket_pool_init(GglSocketPool *pool);

/// Register a fd into a socket pool.
/// If successful, `handle` is populated with a handle for the fd.
GglError ggl_socket_pool_register(
    GglSocketPool *pool, int fd, uint32_t *handle
);

/// Take a fd from a socket pool.
/// If successful, the fd was removed and is now owned by the caller.
GglError ggl_socket_pool_release(GglSocketPool *pool, uint32_t handle, int *fd);

/// Read exact amount of data from a socket.
GglError ggl_socket_handle_read(
    GglSocketPool *pool, uint32_t handle, GglBuffer buf
);

/// Write exact amount of data to a socket.
GglError ggl_socket_handle_write(
    GglSocketPool *pool, uint32_t handle, GglBuffer buf
);

/// Close a socket.
GglError ggl_socket_handle_close(GglSocketPool *pool, uint32_t handle);

/// Get pid of socket peer.
GglError ggl_socket_handle_get_peer_pid(
    GglSocketPool *pool, uint32_t handle, pid_t *pid
);

/// Run action with handle protected and access to the state index.
/// This can be used for managing additional state arrays kept in sync with the
/// socket pool state or to protect the action from concurrent cleanup.
GglError ggl_socket_handle_protected(
    void (*action)(void *ctx, size_t index),
    void *ctx,
    GglSocketPool *pool,
    uint32_t handle
);

typedef struct {
    GglSocketPool *pool;
    uint32_t handle;
} GglSocketHandleReaderCtx;

/// Reader that reads from a stream socket handle.
/// Data may be remaining if buffer is filled.
/// ctx will be overwritten; used for storage.
/// Reader lives as long as ctx.
GglReader ggl_socket_handle_reader(
    GglSocketHandleReaderCtx *ctx, GglSocketPool *pool, uint32_t handle
);

#endif
