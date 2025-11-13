// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_BUFFER_H
#define GG_BUFFER_H

//! Buffer utilities.

#include <gg/attr.h>
#include <gg/cbmc.h>
#include <gg/error.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined __COVERITY__ || defined __CPROVER__
#define GG_DISABLE_MACRO_TYPE_CHECKING
#endif

/// A fixed buffer of bytes. Possibly a string.
typedef struct {
    uint8_t *data;
    size_t len;
} GgBuffer;

/// An array of `GgBuffer`.
typedef struct {
    GgBuffer *bufs;
    size_t len;
} GgBufList;

#define GG_STR(strlit) \
    ((GgBuffer) { .data = (uint8_t *) ("" strlit ""), \
                  .len = sizeof(strlit) - 1U })

// generic function on pointer is to validate parameter is array and not ptr.
// On systems where char == uint8_t, this won't warn on string literal.

#define GG_BUF_UNCHECKED(...) \
    ((GgBuffer) { .data = (__VA_ARGS__), .len = sizeof(__VA_ARGS__) })

#ifndef GG_DISABLE_MACRO_TYPE_CHECKING
/// Create buffer literal from a byte array.
#define GG_BUF(...) \
    _Generic((&(__VA_ARGS__)), uint8_t (*)[]: GG_BUF_UNCHECKED(__VA_ARGS__))
#else
#define GG_BUF GG_BUF_UNCHECKED
#endif

/// Create buffer list literal from buffer literals.
#define GG_BUF_LIST(...) \
    (GgBufList) { \
        .bufs = (GgBuffer[]) { __VA_ARGS__ }, \
        .len = (sizeof((GgBuffer[]) { __VA_ARGS__ })) / (sizeof(GgBuffer)) \
    }

// NOLINTBEGIN(bugprone-macro-parentheses)
/// Loop over the objects in a list.
#define GG_BUF_LIST_FOREACH(name, list) \
    for (GgBuffer *name = (list).bufs; name < &(list).bufs[(list).len]; \
         name = &name[1])
// NOLINTEND(bugprone-macro-parentheses)

#ifdef __CPROVER__
bool cbmc_buffer_restrict(GgBuffer *buf) {
    return (buf->len == 0) || cbmc_restrict(buf->data, buf->len);
}
#endif

/// Convert null-terminated string to buffer.
PURE NULL_TERMINATED_STRING_ARG(1)
GgBuffer gg_buffer_from_null_term(char str[static 1]);

/// Returns whether two buffers have identical content.
PURE
bool gg_buffer_eq(GgBuffer buf1, GgBuffer buf2);

/// Returns whether the buffer has the given prefix.
PURE
bool gg_buffer_has_prefix(GgBuffer buf, GgBuffer prefix);

/// Removes a prefix. Returns whether the prefix was removed.
ACCESS(read_write, 1)
bool gg_buffer_remove_prefix(GgBuffer buf[static 1], GgBuffer prefix);

/// Returns whether the buffer has the given suffix.
PURE
bool gg_buffer_has_suffix(GgBuffer buf, GgBuffer suffix);

/// Removes a suffix. Returns whether the suffix was removed.
ACCESS(read_write, 1)
bool gg_buffer_remove_suffix(GgBuffer buf[static 1], GgBuffer suffix);

/// Returns whether the buffer contains the given substring.
/// Outputs start index if non-null.
ACCESS(write_only, 3) REPRODUCIBLE
bool gg_buffer_contains(GgBuffer buf, GgBuffer substring, size_t *start);

/// Returns substring of buffer from start to end.
/// The result is the overlap between the start to end range and the input
/// bounds.
CONST
GgBuffer gg_buffer_substr(GgBuffer buf, size_t start, size_t end) CBMC_CONTRACT(
    requires(cbmc_buffer_restrict(&buf)),
    requires(start < end),
    ensures(cbmc_implies(
        (buf.data == NULL), (cbmc_return.data == NULL) && (cbmc_return.len == 0)
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
        (buf.data != NULL) && (end <= buf.len), (cbmc_return.len == end - start)
    )),
);

/// Parse an integer from a string.
ACCESS(write_only, 2)
GgError gg_str_to_int64(GgBuffer str, int64_t value[static 1]) CBMC_CONTRACT(
    requires(cbmc_buffer_restrict(&str)),
    requires(cbmc_restrict(value)),
    ensures(cbmc_enum_valid(cbmc_return)),
    assigns(*value)
);

/// Copy source buffer into target buffer.
/// Returns GG_ERR_NOMEM if target does not have source.len bytes.
/// On success, updates `target->len` to source.len and returns GG_ERR_OK.
GgError gg_buf_copy(GgBuffer source, GgBuffer *target);

#endif
