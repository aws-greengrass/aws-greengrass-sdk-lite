// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_BUFFER_H
#define GGL_BUFFER_H

//! Buffer utilities.

#include <ggl/attr.h>
#include <ggl/cbmc.h>
#include <ggl/error.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined __COVERITY__ || defined __CPROVER__
#define GGL_DISABLE_MACRO_TYPE_CHECKING
#endif

/// A fixed buffer of bytes. Possibly a string.
typedef struct {
    uint8_t *data;
    size_t len;
} GglBuffer;

/// An array of `GglBuffer`
typedef struct {
    GglBuffer *bufs;
    size_t len;
} GglBufList;

#define GGL_STR(strlit) \
    ((GglBuffer) { .data = (uint8_t *) ("" strlit ""), \
                   .len = sizeof(strlit) - 1U })

// generic function on pointer is to validate parameter is array and not ptr.
// On systems where char == uint8_t, this won't warn on string literal.

#define GGL_BUF_UNCHECKED(...) \
    ((GglBuffer) { .data = (__VA_ARGS__), .len = sizeof(__VA_ARGS__) })

#ifndef GGL_DISABLE_MACRO_TYPE_CHECKING
/// Create buffer literal from a byte array.
#define GGL_BUF(...) \
    _Generic((&(__VA_ARGS__)), uint8_t(*)[]: GGL_BUF_UNCHECKED(__VA_ARGS__))
#else
#define GGL_BUF GGL_BUF_UNCHECKED
#endif

/// Create buffer list literal from buffer literals.
#define GGL_BUF_LIST(...) \
    (GglBufList) { \
        .bufs = (GglBuffer[]) { __VA_ARGS__ }, \
        .len = (sizeof((GglBuffer[]) { __VA_ARGS__ })) / (sizeof(GglBuffer)) \
    }

// NOLINTBEGIN(bugprone-macro-parentheses)
/// Loop over the objects in a list.
#define GGL_BUF_LIST_FOREACH(name, list) \
    for (GglBuffer *name = (list).bufs; name < &(list).bufs[(list).len]; \
         name = &name[1])
// NOLINTEND(bugprone-macro-parentheses)

#ifdef __CPROVER__
bool cbmc_buffer_restrict(GglBuffer *buf) {
    return (buf->len == 0) || cbmc_restrict(buf->data, buf->len);
}
#endif

/// Convert null-terminated string to buffer
GglBuffer ggl_buffer_from_null_term(char str[static 1]) PURE
NULL_TERMINATED_STRING_ARG(1);

/// Returns whether two buffers have identical content.
bool ggl_buffer_eq(GglBuffer buf1, GglBuffer buf2) PURE;

/// Returns whether the buffer has the given prefix.
bool ggl_buffer_has_prefix(GglBuffer buf, GglBuffer prefix) PURE;

/// Removes a prefix. Returns whether the prefix was removed.
bool ggl_buffer_remove_prefix(GglBuffer buf[static 1], GglBuffer prefix)
    ACCESS(read_write, 1);

/// Returns whether the buffer has the given suffix.
bool ggl_buffer_has_suffix(GglBuffer buf, GglBuffer suffix);

/// Removes a suffix. Returns whether the suffix was removed.
bool ggl_buffer_remove_suffix(GglBuffer buf[static 1], GglBuffer suffix)
    ACCESS(read_write, 1);

/// Returns whether the buffer contains the given substring.
/// Outputs start index if non-null.
bool ggl_buffer_contains(GglBuffer buf, GglBuffer substring, size_t *start)
    ACCESS(write_only, 3);

/// Returns substring of buffer from start to end.
/// The result is the overlap between the start to end range and the input
/// bounds.
GglBuffer ggl_buffer_substr(GglBuffer buf, size_t start, size_t end)
    CONST CBMC_CONTRACT(
        requires(cbmc_buffer_restrict(&buf)),
        requires(start < end),
        ensures(cbmc_implies(
            (buf.data == NULL),
            (cbmc_return.data == NULL) && (cbmc_return.len == 0)
        )),
        ensures(cbmc_implies(
            (buf.data != NULL),
            (cbmc_ptr_in_range(buf.data, cbmc_return.data, buf.data + buf.len)),
            ((cbmc_return.data - buf.data) <= (buf.len - cbmc_return.len))
        )),
        ensures(cbmc_implies(
            (buf.data != NULL) && (start <= buf.len),
            (cbmc_return.data == buf.data + start)
        )),
        ensures(cbmc_implies(
            (buf.data != NULL) && (end <= buf.len),
            (cbmc_return.len == end - start)
        )),
    );

/// Parse an integer from a string
GglError ggl_str_to_int64(GglBuffer str, int64_t value[static 1])
    ACCESS(write_only, 2) CBMC_CONTRACT(
        requires(cbmc_buffer_restrict(&str)),
        requires(cbmc_restrict(value)),
        ensures(cbmc_enum_valid(cbmc_return)),
        assigns(*value)
    );

#endif
