// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_ERROR_H
#define GG_ERROR_H

#include <gg/attr.h>
#include <gg/cbmc.h>

//! GG error codes

/// GG error codes, representing class of error.
typedef enum NODISCARD GgError {
    /// Success
    GG_ERR_OK = 0,
    /// Generic failure
    GG_ERR_FAILURE,
    /// Failure, can be retried
    GG_ERR_RETRY,
    /// Request cannot be handled at the time
    GG_ERR_BUSY,
    /// System is in irrecoverably broken state
    GG_ERR_FATAL,
    /// Request is invalid or malformed
    GG_ERR_INVALID,
    /// Request is unsupported
    GG_ERR_UNSUPPORTED,
    /// Request data invalid
    GG_ERR_PARSE,
    /// Request or data outside of allowable range
    GG_ERR_RANGE,
    /// Insufficient memory
    GG_ERR_NOMEM,
    /// No connection
    GG_ERR_NOCONN,
    /// No more data available
    GG_ERR_NODATA,
    /// Unknown entry or target requested
    GG_ERR_NOENTRY,
    /// Invalid or missing configuration
    GG_ERR_CONFIG,
    /// Received remote error
    GG_ERR_REMOTE,
    /// Expected non-ok status
    GG_ERR_EXPECTED,
    /// Request timed out
    GG_ERR_TIMEOUT,
} GgError;

/// Convert a GgError to a string representation.
/// Returns a static string describing the error.
CONST
const char *gg_strerror(GgError err) CBMC_CONTRACT(
    requires(cbmc_enum_valid(err)), ensures(cbmc_restrict(cbmc_return))
);

#endif
