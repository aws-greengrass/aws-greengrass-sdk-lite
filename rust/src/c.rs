// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]
#![allow(clippy::pedantic)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

pub const GGL_OBJ_NULL: GglObject = GglObject { _private: [0; 11] };
