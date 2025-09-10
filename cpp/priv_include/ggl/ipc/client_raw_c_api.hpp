// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_CLIENT_RAW_HPP
#define GGL_IPC_CLIENT_RAW_HPP

#include "ggl/error.hpp"
#include "ggl/types.hpp"

extern "C" {

typedef GglError GgIpcResultCallback(void *ctx, GglMap result) noexcept;
typedef GglError GgIpcErrorCallback(
    void *ctx, GglBuffer error_code, GglBuffer message
) noexcept;

GglError ggipc_call(
    GglBuffer operation,
    GglBuffer service_model_type,
    GglMap params,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_ctx
) noexcept;
}

#endif
