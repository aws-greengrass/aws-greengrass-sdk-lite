// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_IPC_CLIENT_PRIV_H
#define GG_IPC_CLIENT_PRIV_H

#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/eventstream/decode.h>
#include <gg/object.h>

VISIBILITY(hidden)
GgError ggipc_connect_with_payload(GgBuffer socket_path, GgObject payload);

VISIBILITY(hidden)
GgError ggipc_connect_extra_header_handler(EventStreamHeaderIter headers);

#endif
