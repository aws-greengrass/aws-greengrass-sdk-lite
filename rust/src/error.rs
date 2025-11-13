// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

use crate::c;
use c::GgError::*;
use std::fmt;

/// GG error codes.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(u32)]
pub enum Error {
    /// Generic failure
    Failure = GG_ERR_FAILURE as u32,
    /// Failure, can be retried
    Retry = GG_ERR_RETRY as u32,
    /// Request cannot be handled at the time
    Busy = GG_ERR_BUSY as u32,
    /// System is in irrecoverably broken state
    Fatal = GG_ERR_FATAL as u32,
    /// Request is invalid or malformed
    Invalid = GG_ERR_INVALID as u32,
    /// Request is unsupported
    Unsupported = GG_ERR_UNSUPPORTED as u32,
    /// Request data invalid
    Parse = GG_ERR_PARSE as u32,
    /// Request or data outside of allowable range
    Range = GG_ERR_RANGE as u32,
    /// Insufficient memory
    Nomem = GG_ERR_NOMEM as u32,
    /// No connection
    Noconn = GG_ERR_NOCONN as u32,
    /// No more data available
    Nodata = GG_ERR_NODATA as u32,
    /// Unknown entry or target requested
    Noentry = GG_ERR_NOENTRY as u32,
    /// Invalid or missing configuration
    Config = GG_ERR_CONFIG as u32,
    /// Received remote error
    Remote = GG_ERR_REMOTE as u32,
    /// Expected non-ok status
    Expected = GG_ERR_EXPECTED as u32,
    /// Request timed out
    Timeout = GG_ERR_TIMEOUT as u32,
}

impl std::error::Error for Error {}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let s = unsafe {
            let ptr = c::gg_strerror((*self).into());
            let cstr = std::ffi::CStr::from_ptr(ptr);
            std::str::from_utf8_unchecked(cstr.to_bytes())
        };
        write!(f, "{s}")
    }
}

/// Result type alias using SDK Error.
pub type Result<T> = std::result::Result<T, Error>;

impl From<Error> for c::GgError {
    fn from(val: Error) -> Self {
        match val {
            Error::Failure => GG_ERR_FAILURE,
            Error::Retry => GG_ERR_RETRY,
            Error::Busy => GG_ERR_BUSY,
            Error::Fatal => GG_ERR_FATAL,
            Error::Invalid => GG_ERR_INVALID,
            Error::Unsupported => GG_ERR_UNSUPPORTED,
            Error::Parse => GG_ERR_PARSE,
            Error::Range => GG_ERR_RANGE,
            Error::Nomem => GG_ERR_NOMEM,
            Error::Noconn => GG_ERR_NOCONN,
            Error::Nodata => GG_ERR_NODATA,
            Error::Noentry => GG_ERR_NOENTRY,
            Error::Config => GG_ERR_CONFIG,
            Error::Remote => GG_ERR_REMOTE,
            Error::Expected => GG_ERR_EXPECTED,
            Error::Timeout => GG_ERR_TIMEOUT,
        }
    }
}

impl From<Result<()>> for c::GgError {
    fn from(value: Result<()>) -> Self {
        match value {
            Ok(()) => c::GgError::GG_ERR_OK,
            Err(e) => e.into(),
        }
    }
}

impl From<c::GgError> for Result<()> {
    fn from(err: c::GgError) -> Self {
        match err {
            GG_ERR_OK => Ok(()),
            GG_ERR_FAILURE => Err(Error::Failure),
            GG_ERR_RETRY => Err(Error::Retry),
            GG_ERR_BUSY => Err(Error::Busy),
            GG_ERR_FATAL => Err(Error::Fatal),
            GG_ERR_INVALID => Err(Error::Invalid),
            GG_ERR_UNSUPPORTED => Err(Error::Unsupported),
            GG_ERR_PARSE => Err(Error::Parse),
            GG_ERR_RANGE => Err(Error::Range),
            GG_ERR_NOMEM => Err(Error::Nomem),
            GG_ERR_NOCONN => Err(Error::Noconn),
            GG_ERR_NODATA => Err(Error::Nodata),
            GG_ERR_NOENTRY => Err(Error::Noentry),
            GG_ERR_CONFIG => Err(Error::Config),
            GG_ERR_REMOTE => Err(Error::Remote),
            GG_ERR_EXPECTED => Err(Error::Expected),
            GG_ERR_TIMEOUT => Err(Error::Timeout),
        }
    }
}
