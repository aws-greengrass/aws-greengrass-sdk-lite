// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_SOCKET_H
#define GG_SOCKET_H

//! Socket function wrappers

#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/io.h>

/// Wrapper for reading full buffer from socket.
VISIBILITY(hidden)
GgError gg_socket_read(int fd, GgBuffer buf);

/// Wrapper for writing full buffer to socket.
VISIBILITY(hidden)
GgError gg_socket_write(int fd, GgBuffer buf);

/// Connect to a socket and return the fd
VISIBILITY(hidden)
GgError gg_connect(GgBuffer path, int *fd);

/// Reader that reads from a stream socket.
/// Data may be remaining if buffer is filled.
VISIBILITY(hidden)
GgReader gg_socket_reader(int *fd);

/// Writer that writes to a stream socket.
VISIBILITY(hidden)
GgWriter gg_socket_writer(int *fd);

#endif
