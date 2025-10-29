// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#![warn(clippy::pedantic, clippy::cargo)]
#![allow(clippy::enum_glob_use)]

mod c;
pub mod error;
pub mod ipc;
pub mod object;

pub use error::{Error, Result};
pub use ipc::Sdk;
pub use object::{Kv, KvRef, MapExt, Object, ObjectRef};
