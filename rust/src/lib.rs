// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//! Lightweight AWS IoT Greengrass SDK for making IPC calls.

#![warn(missing_docs, clippy::pedantic, clippy::cargo)]
#![allow(clippy::enum_glob_use)]

mod c;
mod error;
mod ipc;
mod object;

pub use error::{Error, Result};
pub use ipc::{Qos, Sdk, SubscribeToTopicPayload, Subscription};
pub use object::{
    Kv, KvRef, List, ListRef, Map, MapRef, Object, ObjectRef, UnpackedObject,
};
