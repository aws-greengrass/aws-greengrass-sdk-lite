// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_ERROR_HPP
#define GGL_ERROR_HPP

#include <system_error>

//! GGL error codes

extern "C" {
/// GGL error codes, representing class of error.
typedef enum [[nodiscard]] GglError {
    /// Success
    GGL_ERR_OK = 0,
    /// Generic failure
    GGL_ERR_FAILURE,
    /// Failure, can be retried
    GGL_ERR_RETRY,
    /// Request cannot be handled at the time
    GGL_ERR_BUSY,
    /// System is in irrecoverably broken state
    GGL_ERR_FATAL,
    /// Request is invalid or malformed
    GGL_ERR_INVALID,
    /// Request is unsupported
    GGL_ERR_UNSUPPORTED,
    /// Request data invalid
    GGL_ERR_PARSE,
    /// Request or data outside of allowable range
    GGL_ERR_RANGE,
    /// Insufficient memory
    GGL_ERR_NOMEM,
    /// No connection
    GGL_ERR_NOCONN,
    /// No more data available
    GGL_ERR_NODATA,
    /// Unknown entry or target requested
    GGL_ERR_NOENTRY,
    /// Invalid or missing configuration
    GGL_ERR_CONFIG,
    /// Received remote error
    GGL_ERR_REMOTE,
    /// Expected non-ok status
    GGL_ERR_EXPECTED,
    /// Request timed out
    GGL_ERR_TIMEOUT,
} GglError;
}

namespace ggl {
// for differentiation between standard error categories
const std::error_category &category() noexcept;
}

// Allow implicit conversion from GglError to std::error_code and comparisons
// between GglError and std::error_code
namespace std {
template <> struct is_error_code_enum<GglError> : true_type { };
}

// implicit conversion uses make_error_code
inline std::error_code make_error_code(GglError ggl_error_value) noexcept {
    return { static_cast<int>(ggl_error_value), ggl::category() };
}

namespace ggl {
class Exception : public std::system_error {
public:
    Exception(GglError code)
        : std::system_error { code } {
    }

    Exception(GglError code, const std::string &what)
        : std::system_error { code, what } {
    }

    Exception(GglError code, const char *what)
        : std::system_error { code, what } {
    }
};

}

#endif
