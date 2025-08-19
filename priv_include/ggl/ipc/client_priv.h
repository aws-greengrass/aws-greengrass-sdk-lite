// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GGL_IPC_CLIENT_PRIV_H
#define GGL_IPC_CLIENT_PRIV_H

#include <ggl/attr.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/eventstream/decode.h>
#include <ggl/object.h>

VISIBILITY(hidden)
GglError ggipc_connect_with_payload(GglBuffer socket_path, GglObject payload);

VISIBILITY(hidden)
GglError ggipc_connect_extra_header_handler(EventStreamHeaderIter headers);

#endif
