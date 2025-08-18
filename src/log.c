// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <ggl/cleanup.h>
#include <ggl/log.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static bool enable_systemd_log_prefix = false;

__attribute__((constructor)) static void configure_logging(void) {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    if (getenv("JOURNAL_STREAM") != NULL) {
        enable_systemd_log_prefix = true;
    }
}

void ggl_log(
    uint32_t level,
    const char *file,
    int line,
    const char *tag,
    const char *format,
    ...
) {
    static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

    const char *prefix = "";

    if (enable_systemd_log_prefix) {
        switch (level) {
        case GGL_LOG_ERROR:
            prefix = "<3>";
            break;
        case GGL_LOG_WARN:
            prefix = "<4>";
            break;
        case GGL_LOG_INFO:
            prefix = "<6>";
            break;
        case GGL_LOG_DEBUG:
        case GGL_LOG_TRACE:
            prefix = "<7>";
            break;
        default:
            break;
        }
    }

    const char *level_str;
    switch (level) {
    case GGL_LOG_ERROR:
        level_str = "\033[1;31mE";
        break;
    case GGL_LOG_WARN:
        level_str = "\033[1;33mW";
        break;
    case GGL_LOG_INFO:
        level_str = "\033[0;32mI";
        break;
    case GGL_LOG_DEBUG:
        level_str = "\033[0;34mD";
        break;
    case GGL_LOG_TRACE:
        level_str = "\033[0;37mT";
        break;
    default:
        level_str = "\033[0;37m?";
    }

    {
        GGL_MTX_SCOPE_GUARD(&log_mutex);

        fprintf(stderr, "%s%s[%s] %s:%d: ", prefix, level_str, tag, file, line);

        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);

        fprintf(stderr, "\033[0m\n");
        fflush(stderr);
    }
}
