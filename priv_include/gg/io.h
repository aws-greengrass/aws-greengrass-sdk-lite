// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_IO_H
#define GG_IO_H

//! Reader/Writer abstractions

#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <stdio.h>

/// Abstraction for streaming data into
typedef struct {
    GgError (*write)(void *ctx, GgBuffer buf);
    void *ctx;
} GgWriter;

/// Write to a GgWriter
static inline GgError gg_writer_call(GgWriter writer, GgBuffer buf) {
    if (writer.write == NULL) {
        return buf.len == 0 ? GG_ERR_OK : GG_ERR_FAILURE;
    }
    return writer.write(writer.ctx, buf);
}

/// Writer that 0 bytes can be written to
#define GG_NULL_WRITER (GgWriter) { 0 }

/// Abstraction for streaming data from
/// Updates buf to amount read
/// `read` must fill the buffer as much as possible; if less than buf's original
/// length is read, the data is complete.
/// APIs may require a reader that errors if the data does not fit into buffer
/// or may require a reader that can be read multiple times until complete.
typedef struct {
    GgError (*read)(void *ctx, GgBuffer *buf);
    void *ctx;
} GgReader;

/// Read from a GgReader
static inline GgError gg_reader_call(GgReader reader, GgBuffer *buf) {
    if (reader.read == NULL) {
        buf->len = 0;
        return GG_ERR_OK;
    }
    return reader.read(reader.ctx, buf);
}

/// Fill entire buffer from a GgReader
static inline GgError gg_reader_call_exact(GgReader reader, GgBuffer buf) {
    GgBuffer copy = buf;
    GgError ret = gg_reader_call(reader, &copy);
    if (ret != GG_ERR_OK) {
        return ret;
    }
    return copy.len == buf.len ? GG_ERR_OK : GG_ERR_FAILURE;
}

/// Reader that 0 bytes can be read from
#define GG_NULL_READER (GgReader) { 0 }

/// Returns a writer that writes into a buffer, consuming used portion
VISIBILITY(hidden)
GgWriter gg_buf_writer(GgBuffer *buf);

#endif
