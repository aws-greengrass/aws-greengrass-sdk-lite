// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

use crate::c;
use c::GglError::*;
use std::fmt;

/// GGL error codes.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(u32)]
pub enum Error {
    /// Generic failure
    Failure = GGL_ERR_FAILURE as u32,
    /// Failure, can be retried
    Retry = GGL_ERR_RETRY as u32,
    /// Request cannot be handled at the time
    Busy = GGL_ERR_BUSY as u32,
    /// System is in irrecoverably broken state
    Fatal = GGL_ERR_FATAL as u32,
    /// Request is invalid or malformed
    Invalid = GGL_ERR_INVALID as u32,
    /// Request is unsupported
    Unsupported = GGL_ERR_UNSUPPORTED as u32,
    /// Request data invalid
    Parse = GGL_ERR_PARSE as u32,
    /// Request or data outside of allowable range
    Range = GGL_ERR_RANGE as u32,
    /// Insufficient memory
    Nomem = GGL_ERR_NOMEM as u32,
    /// No connection
    Noconn = GGL_ERR_NOCONN as u32,
    /// No more data available
    Nodata = GGL_ERR_NODATA as u32,
    /// Unknown entry or target requested
    Noentry = GGL_ERR_NOENTRY as u32,
    /// Invalid or missing configuration
    Config = GGL_ERR_CONFIG as u32,
    /// Received remote error
    Remote = GGL_ERR_REMOTE as u32,
    /// Expected non-ok status
    Expected = GGL_ERR_EXPECTED as u32,
    /// Request timed out
    Timeout = GGL_ERR_TIMEOUT as u32,
}

impl std::error::Error for Error {}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let s = unsafe {
            let ptr = c::ggl_strerror((*self).into());
            let cstr = std::ffi::CStr::from_ptr(ptr);
            std::str::from_utf8_unchecked(cstr.to_bytes())
        };
        write!(f, "{s}")
    }
}

/// Result type alias using SDK Error.
pub type Result<T> = std::result::Result<T, Error>;

impl From<Error> for c::GglError {
    fn from(val: Error) -> Self {
        match val {
            Error::Failure => GGL_ERR_FAILURE,
            Error::Retry => GGL_ERR_RETRY,
            Error::Busy => GGL_ERR_BUSY,
            Error::Fatal => GGL_ERR_FATAL,
            Error::Invalid => GGL_ERR_INVALID,
            Error::Unsupported => GGL_ERR_UNSUPPORTED,
            Error::Parse => GGL_ERR_PARSE,
            Error::Range => GGL_ERR_RANGE,
            Error::Nomem => GGL_ERR_NOMEM,
            Error::Noconn => GGL_ERR_NOCONN,
            Error::Nodata => GGL_ERR_NODATA,
            Error::Noentry => GGL_ERR_NOENTRY,
            Error::Config => GGL_ERR_CONFIG,
            Error::Remote => GGL_ERR_REMOTE,
            Error::Expected => GGL_ERR_EXPECTED,
            Error::Timeout => GGL_ERR_TIMEOUT,
        }
    }
}

impl From<Result<()>> for c::GglError {
    fn from(value: Result<()>) -> Self {
        match value {
            Ok(()) => c::GglError::GGL_ERR_OK,
            Err(e) => e.into(),
        }
    }
}

impl From<c::GglError> for Result<()> {
    fn from(err: c::GglError) -> Self {
        match err {
            GGL_ERR_OK => Ok(()),
            GGL_ERR_FAILURE => Err(Error::Failure),
            GGL_ERR_RETRY => Err(Error::Retry),
            GGL_ERR_BUSY => Err(Error::Busy),
            GGL_ERR_FATAL => Err(Error::Fatal),
            GGL_ERR_INVALID => Err(Error::Invalid),
            GGL_ERR_UNSUPPORTED => Err(Error::Unsupported),
            GGL_ERR_PARSE => Err(Error::Parse),
            GGL_ERR_RANGE => Err(Error::Range),
            GGL_ERR_NOMEM => Err(Error::Nomem),
            GGL_ERR_NOCONN => Err(Error::Noconn),
            GGL_ERR_NODATA => Err(Error::Nodata),
            GGL_ERR_NOENTRY => Err(Error::Noentry),
            GGL_ERR_CONFIG => Err(Error::Config),
            GGL_ERR_REMOTE => Err(Error::Remote),
            GGL_ERR_EXPECTED => Err(Error::Expected),
            GGL_ERR_TIMEOUT => Err(Error::Timeout),
        }
    }
}
