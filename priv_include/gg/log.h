// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_LOG_H
#define GG_LOG_H

//! Logging interface

#include <gg/attr.h>
#include <gg/cbmc.h>
#include <stdint.h>

/// Logging interface implementation.
/// Do not call directly; use one of the macro wrappers.
/// Default implementation prints to stderr.
VISIBILITY(hidden) FORMAT(printf, 5, 6)
void gg_log(
    uint32_t level,
    const char *file,
    int line,
    const char *tag,
    const char *format,
    ...
) CBMC_CONTRACT(requires(cbmc_restrict(format)));

/// No-op logging fn for enabling type checking disabled logging macros.
ALWAYS_INLINE FORMAT(printf, 1, 2)
static inline void gg_log_disabled(const char *format, ...) {
    (void) format;
}

#define GG_LOG_NONE 0
#define GG_LOG_ERROR 1
#define GG_LOG_WARN 2
#define GG_LOG_INFO 3
#define GG_LOG_DEBUG 4
#define GG_LOG_TRACE 5

/// Minimum log level to print.
/// Can be overridden from make using command line or environment.
#ifndef GG_LOG_LEVEL
#define GG_LOG_LEVEL GG_LOG_INFO
#endif

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

#define GG_LOG(level, ...) \
    gg_log(level, __FILE_NAME__, __LINE__, GG_MODULE, __VA_ARGS__)

#if GG_LOG_LEVEL >= GG_LOG_ERROR
#define GG_LOGE(...) GG_LOG(GG_LOG_ERROR, __VA_ARGS__)
#else
#define GG_LOGE(...) gg_log_disabled(__VA_ARGS__)
#endif

#if GG_LOG_LEVEL >= GG_LOG_WARN
#define GG_LOGW(...) GG_LOG(GG_LOG_WARN, __VA_ARGS__)
#else
#define GG_LOGW(...) gg_log_disabled(__VA_ARGS__)
#endif

#if GG_LOG_LEVEL >= GG_LOG_INFO
#define GG_LOGI(...) GG_LOG(GG_LOG_INFO, __VA_ARGS__)
#else
#define GG_LOGI(...) gg_log_disabled(__VA_ARGS__)
#endif

#if GG_LOG_LEVEL >= GG_LOG_DEBUG
#define GG_LOGD(...) GG_LOG(GG_LOG_DEBUG, __VA_ARGS__)
#else
#define GG_LOGD(...) gg_log_disabled(__VA_ARGS__)
#endif

#if GG_LOG_LEVEL >= GG_LOG_TRACE
#define GG_LOGT(...) GG_LOG(GG_LOG_TRACE, __VA_ARGS__)
#else
#define GG_LOGT(...) gg_log_disabled(__VA_ARGS__)
#endif

#endif
