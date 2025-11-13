// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <gg/error.h>
#include <stdbool.h>

const char *gg_strerror(GgError err) {
    switch (err) {
    case GG_ERR_OK:
        return "OK";
    case GG_ERR_FAILURE:
        return "FAILURE";
    case GG_ERR_RETRY:
        return "RETRY";
    case GG_ERR_BUSY:
        return "BUSY";
    case GG_ERR_FATAL:
        return "FATAL";
    case GG_ERR_INVALID:
        return "INVALID";
    case GG_ERR_UNSUPPORTED:
        return "UNSUPPORTED";
    case GG_ERR_PARSE:
        return "PARSE";
    case GG_ERR_RANGE:
        return "RANGE";
    case GG_ERR_NOMEM:
        return "NOMEM";
    case GG_ERR_NOCONN:
        return "NOCONN";
    case GG_ERR_NODATA:
        return "NODATA";
    case GG_ERR_NOENTRY:
        return "NOENTRY";
    case GG_ERR_CONFIG:
        return "CONFIG";
    case GG_ERR_REMOTE:
        return "REMOTE";
    case GG_ERR_EXPECTED:
        return "EXPECTED";
    case GG_ERR_TIMEOUT:
        return "TIMEOUT";
    }

    assert(false);
    return "";
}
