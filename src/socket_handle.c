// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <ggl/buffer.h>
#include <ggl/cleanup.h>
#include <ggl/error.h>
#include <ggl/file.h>
#include <ggl/io.h>
#include <ggl/log.h>
#include <ggl/socket_handle.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

// Handles are 32 bits, with the high 16 bits being a generation counter, and
// the low 16 bits being an offset index. The generation counter is incremented
// on close, to prevent reuse.
//
// Use of the index and generation count must be done with a mutex held to
// prevent concurrent incrementing of the generation counter.
//
// The index is offset by 1 in order to ensure 0 is not a valid handle,
// preventing a zero-initialized handle from accidentally working. Since the
// array length (pool->max_fds) is in the range [0, UINT16_MAX], valid indices
// are in the range [0, UINT16_MAX - 1]. Thus incrementing the index will not
// overflow a uint16_t.

static const int32_t FD_FREE = -0x55555556; // Alternating bits for debugging

static GglError validate_handle(
    GglSocketPool *pool, uint32_t handle, uint16_t *index, const char *location
) {
    // Underflow ok; UINT16_MAX will fail bounds check
    uint16_t handle_index = (uint16_t) ((handle & UINT16_MAX) - 1U);
    uint16_t handle_generation = (uint16_t) (handle >> 16);

    if (handle_index >= pool->max_fds) {
        GGL_LOGE("Invalid handle %u in %s.", handle, location);
        return GGL_ERR_INVALID;
    }

    if (handle_generation != pool->generations[handle_index]) {
        GGL_LOGD("Generation mismatch for handle %d in %s.", handle, location);
        return GGL_ERR_NOENTRY;
    }

    *index = handle_index;
    return GGL_ERR_OK;
}

void ggl_socket_pool_init(GglSocketPool *pool) {
    assert(pool != NULL);
    assert(pool->fds != NULL);
    assert(pool->generations != NULL);

    GGL_LOGT("Initializing socket pool %p.", pool);

    for (size_t i = 0; i < pool->max_fds; i++) {
        pool->fds[i] = FD_FREE;
    }

    // TODO: handle mutex init failure?
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&pool->mtx, &attr);
}

GglError ggl_socket_pool_register(
    GglSocketPool *pool, int fd, uint32_t *handle
) {
    assert(handle != NULL);

    GGL_LOGT("Registering fd %d in pool %p.", fd, pool);

    if (fd < 0) {
        GGL_LOGE("%s received invalid fd: %d.", __func__, fd);
        return GGL_ERR_INVALID;
    }

    GGL_MTX_SCOPE_GUARD(&pool->mtx);

    for (uint16_t i = 0; i < pool->max_fds; i++) {
        if (pool->fds[i] == FD_FREE) {
            pool->fds[i] = fd;
            uint32_t new_handle
                = (uint32_t) pool->generations[i] << 16 | (i + 1U);

            if (pool->on_register != NULL) {
                GglError ret = pool->on_register(new_handle, i);
                if (ret != GGL_ERR_OK) {
                    pool->fds[i] = FD_FREE;
                    GGL_LOGE("Pool on_register callback failed.");
                    return ret;
                }
            }

            *handle = new_handle;

            GGL_LOGD(
                "Registered fd %d at index %u, generation %u with handle %u.",
                fd,
                i,
                pool->generations[i],
                new_handle
            );

            // coverity[missing_restore]
            return GGL_ERR_OK;
        }
    }

    GGL_LOGE("Pool maximum fds exceeded.");
    return GGL_ERR_NOMEM;
}

GglError ggl_socket_pool_release(
    GglSocketPool *pool, uint32_t handle, int *fd
) {
    GGL_LOGT("Releasing handle %u in pool %p.", handle, pool);

    GGL_MTX_SCOPE_GUARD(&pool->mtx);

    uint16_t index = 0;
    GglError ret = validate_handle(pool, handle, &index, __func__);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    if (pool->on_release != NULL) {
        ret = pool->on_release(handle, index);
        if (ret != GGL_ERR_OK) {
            GGL_LOGE(
                "Pool on_release callback failed for fd %d, index %u, "
                "generation %u.",
                pool->fds[index],
                index,
                pool->generations[index]
            );
            return ret;
        }
    }

    if (fd != NULL) {
        *fd = pool->fds[index];
    }

    GGL_LOGD(
        "Releasing fd %d at index %u, generation %u.",
        pool->fds[index],
        index,
        pool->generations[index]
    );

    pool->generations[index] += 1;
    pool->fds[index] = FD_FREE;

    return GGL_ERR_OK;
}

GglError ggl_socket_handle_read(
    GglSocketPool *pool, uint32_t handle, GglBuffer buf
) {
    GGL_LOGT(
        "Reading %zu bytes from handle %u in pool %p.", buf.len, handle, pool
    );

    GglBuffer rest = buf;

    while (rest.len > 0) {
        GGL_MTX_SCOPE_GUARD(&pool->mtx);

        uint16_t index = 0;
        GglError ret = validate_handle(pool, handle, &index, __func__);
        if (ret != GGL_ERR_OK) {
            return ret;
        }

        ret = ggl_file_read_partial(pool->fds[index], &rest);
        if (ret == GGL_ERR_RETRY) {
            continue;
        }
        if (ret != GGL_ERR_OK) {
            return ret;
        }
    }

    GGL_LOGT("Read from %u successful.", handle);
    return GGL_ERR_OK;
}

GglError ggl_socket_handle_write(
    GglSocketPool *pool, uint32_t handle, GglBuffer buf
) {
    GGL_LOGT(
        "Writing %zu bytes to handle %u in pool %p.", buf.len, handle, pool
    );

    GglBuffer rest = buf;

    while (rest.len > 0) {
        GGL_MTX_SCOPE_GUARD(&pool->mtx);

        uint16_t index = 0;
        GglError ret = validate_handle(pool, handle, &index, __func__);
        if (ret != GGL_ERR_OK) {
            return ret;
        }

        ret = ggl_file_write_partial(pool->fds[index], &rest);
        if (ret == GGL_ERR_RETRY) {
            continue;
        }
        if (ret != GGL_ERR_OK) {
            return ret;
        }
    }

    GGL_LOGT("Write to %u successful.", handle);
    return GGL_ERR_OK;
}

GglError ggl_socket_handle_close(GglSocketPool *pool, uint32_t handle) {
    GGL_LOGT("Closing handle %u in pool %p.", handle, pool);

    int fd = -1;

    GglError ret = ggl_socket_pool_release(pool, handle, &fd);
    if (ret == GGL_ERR_OK) {
        (void) ggl_close(fd);
    }

    GGL_LOGT("Close of %u successful.", handle);
    return ret;
}

GglError ggl_socket_handle_get_peer_pid(
    GglSocketPool *pool, uint32_t handle, pid_t *pid
) {
    GGL_LOGT("Getting peer pid for handle %u in pool %p.", handle, pool);

    GGL_MTX_SCOPE_GUARD(&pool->mtx);

    uint16_t index = 0;
    GglError ret = validate_handle(pool, handle, &index, __func__);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    struct ucred ucred;
    socklen_t ucred_len = sizeof(ucred);
    if ((getsockopt(
             pool->fds[index], SOL_SOCKET, SO_PEERCRED, &ucred, &ucred_len
         )
         != 0)
        || (ucred_len != sizeof(ucred))) {
        GGL_LOGE("Failed to get peer cred for fd %d.", pool->fds[index]);
        return GGL_ERR_FAILURE;
    }

    *pid = ucred.pid;
    GGL_LOGT("Get pid for %u successful (%d).", handle, ucred.pid);
    return GGL_ERR_OK;
}

GglError ggl_socket_handle_protected(
    void (*action)(void *ctx, size_t index),
    void *ctx,
    GglSocketPool *pool,
    uint32_t handle
) {
    GGL_LOGT("In %s with handle %u in pool %p.", __func__, handle, pool);

    GGL_MTX_SCOPE_GUARD(&pool->mtx);

    uint16_t index = 0;
    GglError ret = validate_handle(pool, handle, &index, __func__);
    if (ret != GGL_ERR_OK) {
        return ret;
    }

    action(ctx, index);

    GGL_LOGT(
        "Successfully completed %s with handle %u in pool %p.",
        __func__,
        handle,
        pool
    );
    return GGL_ERR_OK;
}

static GglError socket_handle_reader_fn(void *ctx, GglBuffer *buf) {
    GglSocketHandleReaderCtx *args = ctx;
    return ggl_socket_handle_read(args->pool, args->handle, *buf);
}

GglReader ggl_socket_handle_reader(
    GglSocketHandleReaderCtx *ctx, GglSocketPool *pool, uint32_t handle
) {
    ctx->pool = pool;
    ctx->handle = handle;
    return (GglReader) { .read = socket_handle_reader_fn, .ctx = ctx };
}
