// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_SOCKET_H
#define GGL_SOCKET_H

//! Socket function wrappers

#include <ggl/attr.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/io.h>

/// Wrapper for reading full buffer from socket.
VISIBILITY(hidden)
GglError ggl_socket_read(int fd, GglBuffer buf);

/// Wrapper for writing full buffer to socket.
VISIBILITY(hidden)
GglError ggl_socket_write(int fd, GglBuffer buf);

/// Connect to a socket and return the fd
VISIBILITY(hidden)
GglError ggl_connect(GglBuffer path, int *fd);

/// Reader that reads from a stream socket.
/// Data may be remaining if buffer is filled.
VISIBILITY(hidden)
GglReader ggl_socket_reader(int *fd);

/// Writer that writes to a stream socket.
VISIBILITY(hidden)
GglWriter ggl_socket_writer(int *fd);

#endif
