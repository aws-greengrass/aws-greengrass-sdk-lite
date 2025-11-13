// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gg/cleanup.h>
#include <gg/log.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static bool enable_systemd_log_prefix = false;

__attribute__((constructor)) static void configure_logging(void) {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const char *journal_stream = getenv("JOURNAL_STREAM");
    if (journal_stream == NULL) {
        return;
    }

    struct stat stderr_stat;
    if (fstat(STDERR_FILENO, &stderr_stat) != 0) {
        return;
    }

    // Parse JOURNAL_STREAM format: "device:inode"
    char *endptr;
    unsigned long journal_dev = strtoul(journal_stream, &endptr, 10);
    if (*endptr != ':') {
        return;
    }
    unsigned long journal_ino = strtoul(endptr + 1, NULL, 10);

    // Check if stderr matches the journal stream
    if (stderr_stat.st_dev == journal_dev
        && stderr_stat.st_ino == journal_ino) {
        enable_systemd_log_prefix = true;
    }
}

void gg_log(
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
        case GG_LOG_ERROR:
            prefix = "<3>";
            break;
        case GG_LOG_WARN:
            prefix = "<4>";
            break;
        case GG_LOG_INFO:
            prefix = "<6>";
            break;
        case GG_LOG_DEBUG:
        case GG_LOG_TRACE:
            prefix = "<7>";
            break;
        default:
            break;
        }
    }

    unsigned char level_c;
    switch (level) {
    case GG_LOG_ERROR:
        level_c = 'E';
        break;
    case GG_LOG_WARN:
        level_c = 'W';
        break;
    case GG_LOG_INFO:
        level_c = 'I';
        break;
    case GG_LOG_DEBUG:
        level_c = 'D';
        break;
    case GG_LOG_TRACE:
        level_c = 'T';
        break;
    default:
        level_c = '?';
    }

    {
        GG_MTX_SCOPE_GUARD(&log_mutex);

        fprintf(stderr, "%s%c[%s] %s:%d: ", prefix, level_c, tag, file, line);

        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);

        fprintf(stderr, "\n");
        fflush(stderr);
    }
}
