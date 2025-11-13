// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_FILE_H
#define GG_FILE_H

//! File system functionality

#include <dirent.h>
#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>

/// Call close on a fd, handling EINTR
VISIBILITY(hidden)
GgError gg_close(int fd);

/// Cleanup function for closing file descriptors.
static inline void cleanup_close(const int *fd) {
    if (*fd >= 0) {
        (void) gg_close(*fd);
    }
}

/// Cleanup function for closing C file handles (e.g. opened by fdopen)
static inline void cleanup_fclose(FILE **fp) {
    if (*fp != NULL) {
        fclose(*fp);
    }
}

/// Call fsync on an file/dir, handling EINTR
VISIBILITY(hidden)
GgError gg_fsync(int fd);

/// Open a directory, creating it if create is set.
VISIBILITY(hidden)
GgError gg_dir_open(GgBuffer path, int flags, bool create, int *fd);

/// Open a directory under dirfd, creating it if needed
/// If create is true tries calling mkdir for missing dirs, and dirfd must not
/// be O_PATH.
VISIBILITY(hidden)
GgError gg_dir_openat(
    int dirfd, GgBuffer path, int flags, bool create, int *fd
);

/// Open a file under dirfd
VISIBILITY(hidden)
GgError gg_file_openat(
    int dirfd, GgBuffer path, int flags, mode_t mode, int *fd
);

/// Open a file
VISIBILITY(hidden)
GgError gg_file_open(GgBuffer path, int flags, mode_t mode, int *fd);

/// Read from file.
/// If result buf value is less than the input size, the file has ended.
VISIBILITY(hidden)
GgError gg_file_read(int fd, GgBuffer *buf);

/// Read from file, returning error if file ends before buf is full.
VISIBILITY(hidden)
GgError gg_file_read_exact(int fd, GgBuffer buf);

/// Read portion of data from file (makes single read call).
/// Returns remaining buffer.
/// Caller must handle GG_ERR_RETRY and GG_ERR_NODATA
VISIBILITY(hidden)
GgError gg_file_read_partial(int fd, GgBuffer *buf);

/// Write buffer to file.
VISIBILITY(hidden)
GgError gg_file_write(int fd, GgBuffer buf);

/// Write portion of buffer to file (makes single write call).
/// Returns remaining buffer.
/// Caller must handle GG_ERR_RETRY
VISIBILITY(hidden)
GgError gg_file_write_partial(int fd, GgBuffer *buf);

/// Read file contents from path
VISIBILITY(hidden)
GgError gg_file_read_path(GgBuffer path, GgBuffer *content);

/// Read file contents from path under dirfd
VISIBILITY(hidden)
GgError gg_file_read_path_at(int dirfd, GgBuffer path, GgBuffer *content);

static inline void cleanup_closedir(DIR **dirp) {
    if (*dirp != NULL) {
        closedir(*dirp);
    }
}

#endif
