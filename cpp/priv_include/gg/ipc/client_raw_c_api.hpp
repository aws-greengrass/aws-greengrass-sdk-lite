// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_IPC_CLIENT_RAW_HPP
#define GG_IPC_CLIENT_RAW_HPP

#include <gg/error.hpp>
#include <gg/types.hpp>

extern "C" {

typedef GgError GgIpcResultCallback(void *ctx, GgMap result) noexcept;
typedef GgError GgIpcErrorCallback(
    void *ctx, GgBuffer error_code, GgBuffer message
) noexcept;

GgError ggipc_call(
    GgBuffer operation,
    GgBuffer service_model_type,
    GgMap params,
    GgIpcResultCallback *result_callback,
    GgIpcErrorCallback *error_callback,
    void *response_ctx
) noexcept;
}

#endif
