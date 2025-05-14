// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <ggl/error.h>
#include <stdbool.h>

const char *ggl_strerror(GglError err) {
    switch (err) {
    case GGL_ERR_OK:
        return "OK";
    case GGL_ERR_FAILURE:
        return "FAILURE";
    case GGL_ERR_RETRY:
        return "RETRY";
    case GGL_ERR_BUSY:
        return "BUSY";
    case GGL_ERR_FATAL:
        return "FATAL";
    case GGL_ERR_INVALID:
        return "INVALID";
    case GGL_ERR_UNSUPPORTED:
        return "UNSUPPORTED";
    case GGL_ERR_PARSE:
        return "PARSE";
    case GGL_ERR_RANGE:
        return "RANGE";
    case GGL_ERR_NOMEM:
        return "NOMEM";
    case GGL_ERR_NOCONN:
        return "NOCONN";
    case GGL_ERR_NODATA:
        return "NODATA";
    case GGL_ERR_NOENTRY:
        return "NOENTRY";
    case GGL_ERR_CONFIG:
        return "CONFIG";
    case GGL_ERR_REMOTE:
        return "REMOTE";
    case GGL_ERR_EXPECTED:
        return "EXPECTED";
    case GGL_ERR_TIMEOUT:
        return "TIMEOUT";
    }

    assert(false);
    return "";
}
