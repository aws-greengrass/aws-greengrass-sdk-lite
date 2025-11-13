// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_ERROR_HPP
#define GG_ERROR_HPP

#include <string>
#include <system_error>
#include <type_traits>

//! GG error codes

extern "C" {
/// GG error codes, representing class of error.
// NOLINTNEXTLINE(performance-enum-size)
typedef enum [[nodiscard]] GgError {
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
}

namespace gg {
// for differentiation between standard error categories
const std::error_category &category() noexcept;
}

// Allow implicit conversion from GgError to std::error_code and comparisons
// between GgError and std::error_code
namespace std {
template <> struct is_error_code_enum<GgError> : true_type { };
}

// implicit conversion uses make_error_code
inline std::error_code make_error_code(GgError gg_error_value) noexcept {
    return { static_cast<int>(gg_error_value), gg::category() };
}

namespace gg {
class Exception : public std::system_error {
public:
    Exception(GgError code)
        : std::system_error { code } {
    }

    Exception(GgError code, const std::string &what)
        : std::system_error { code, what } {
    }

    Exception(GgError code, const char *what)
        : std::system_error { code, what } {
    }
};

}

#endif
