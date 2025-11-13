// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

use std::{
    env, ffi,
    marker::PhantomData,
    mem, ptr, result, slice, str,
    sync::{Mutex, OnceLock},
};

use crate::{
    c,
    error::{Error, Result},
    object::{KvRef, MapRef, ObjectRef},
};

static INIT: OnceLock<()> = OnceLock::new();
static CONNECTED: Mutex<bool> = Mutex::new(false);

/// AWS IoT Greengrass IPC SDK client.
#[non_exhaustive]
#[derive(Debug, Clone, Copy)]
pub struct Sdk {}

#[derive(Debug, Clone, Copy)]
pub struct IpcError<'a> {
    pub error_code: &'a str,
    pub message: &'a str,
}

/// MQTT Quality of Service level.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Qos {
    /// At most once delivery (QoS 0)
    AtMostOnce = 0,
    /// At least once delivery (QoS 1)
    AtLeastOnce = 1,
}

/// Component lifecycle state.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ComponentState {
    /// Component is running
    Running,
    /// Component encountered an error
    Errored,
}

/// Payload received from a topic subscription.
#[derive(Debug, Clone, Copy)]
pub enum SubscribeToTopicPayload<'a> {
    /// JSON payload
    Json(MapRef<'a>),
    /// Binary payload
    Binary(&'a [u8]),
}

impl Sdk {
    /// Initialize the SDK.
    ///
    /// Must be called before using any IPC operations.
    pub fn init() -> Self {
        INIT.get_or_init(|| unsafe { c::gg_sdk_init() });
        Self {}
    }

    /// Connect to the AWS IoT Greengrass Core IPC service.
    ///
    /// Uses `SVCUID` and `AWS_GG_NUCLEUS_DOMAIN_SOCKET_FILEPATH_FOR_COMPONENT`
    /// environment variables set by the Greengrass nucleus.
    ///
    /// # Errors
    /// Returns error if environment variables are missing, already connected, or connection fails.
    pub fn connect(&self) -> Result<()> {
        let svcuid = env::var("SVCUID").map_err(|_| Error::Config)?;
        let socket_path =
            env::var("AWS_GG_NUCLEUS_DOMAIN_SOCKET_FILEPATH_FOR_COMPONENT")
                .map_err(|_| Error::Config)?;

        self.connect_with_token(&socket_path, &svcuid)
    }

    /// Connect to the AWS IoT Greengrass Core IPC service with explicit credentials.
    ///
    /// # Errors
    /// Returns error if already connected or connection fails.
    #[expect(clippy::missing_panics_doc)]
    pub fn connect_with_token(
        &self,
        socket_path: &str,
        auth_token: &str,
    ) -> Result<()> {
        let mut connected = CONNECTED.lock().unwrap();
        if *connected {
            return Err(Error::Failure);
        }

        let socket_buf = c::GgBuffer {
            data: socket_path.as_ptr().cast_mut(),
            len: socket_path.len(),
        };
        let token_buf = c::GgBuffer {
            data: auth_token.as_ptr().cast_mut(),
            len: auth_token.len(),
        };

        Result::from(unsafe {
            c::ggipc_connect_with_token(socket_buf, token_buf)
        })?;

        *connected = true;
        Ok(())
    }

    /// Publish a JSON message to a local pub/sub topic.
    ///
    /// Sends messages to other Greengrass components subscribed to the topic.
    /// Requires `aws.greengrass#PublishToTopic` authorization.
    ///
    /// See: <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-publish-subscribe.html#ipc-operation-publishtotopic>
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use gg_sdk::{Sdk, Kv, Object};
    ///
    /// let sdk = Sdk::init();
    /// sdk.connect()?;
    ///
    /// let payload = [
    ///     Kv::new("temperature", Object::f64(72.5)),
    ///     Kv::new("humidity", Object::i64(45)),
    /// ];
    /// sdk.publish_to_topic_json("sensor/data", &payload)?;
    /// # Ok::<(), gg_sdk::Error>(())
    /// ```
    ///
    /// # Errors
    /// Returns error if publish fails.
    pub fn publish_to_topic_json<'a>(
        &self,
        topic: impl Into<&'a str>,
        payload: impl Into<MapRef<'a>>,
    ) -> Result<()> {
        fn inner(topic: &str, payload: &MapRef<'_>) -> Result<()> {
            let topic_buf = c::GgBuffer {
                data: topic.as_ptr().cast_mut(),
                len: topic.len(),
            };
            let payload_map = c::GgMap {
                pairs: payload.0.as_ptr() as *mut c::GgKV,
                len: payload.0.len(),
            };

            Result::from(unsafe {
                c::ggipc_publish_to_topic_json(topic_buf, payload_map)
            })
        }
        inner(topic.into(), &payload.into())
    }

    /// Publish a binary message to a local pub/sub topic.
    ///
    /// Sends messages to other Greengrass components subscribed to the topic.
    /// Requires `aws.greengrass#PublishToTopic` authorization.
    ///
    /// See: <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-publish-subscribe.html#ipc-operation-publishtotopic>
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use gg_sdk::Sdk;
    ///
    /// let sdk = Sdk::init();
    /// sdk.connect()?;
    ///
    /// let data = b"binary payload data";
    /// sdk.publish_to_topic_binary("sensor/raw", data)?;
    /// # Ok::<(), gg_sdk::Error>(())
    /// ```
    ///
    /// # Errors
    /// Returns error if publish fails.
    pub fn publish_to_topic_binary<'a>(
        &self,
        topic: impl Into<&'a str>,
        payload: impl AsRef<[u8]>,
    ) -> Result<()> {
        fn inner(topic: &str, payload: &[u8]) -> Result<()> {
            let topic_buf = c::GgBuffer {
                data: topic.as_ptr().cast_mut(),
                len: topic.len(),
            };
            let payload_buf = c::GgBuffer {
                data: payload.as_ptr().cast_mut(),
                len: payload.len(),
            };

            Result::from(unsafe {
                c::ggipc_publish_to_topic_binary(topic_buf, payload_buf)
            })
        }
        inner(topic.into(), payload.as_ref())
    }

    /// Subscribe to messages on a local pub/sub topic.
    ///
    /// Receives messages from other Greengrass components publishing to the topic.
    /// Requires `aws.greengrass#SubscribeToTopic` authorization.
    ///
    /// See: <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-publish-subscribe.html#ipc-operation-subscribetotopic>
    ///
    /// # Errors
    /// Returns error if subscription fails.
    pub fn subscribe_to_topic<
        'a,
        F: FnMut(&str, SubscribeToTopicPayload) + 'a,
    >(
        &self,
        topic: impl Into<&'a str>,
        callback: F,
    ) -> Result<Subscription<'a, F>> {
        extern "C" fn trampoline<F: FnMut(&str, SubscribeToTopicPayload)>(
            ctx: *mut ffi::c_void,
            topic: c::GgBuffer,
            payload: c::GgObject,
            _handle: c::GgIpcSubscriptionHandle,
        ) {
            let cb = unsafe { &mut *ctx.cast::<F>() };
            let topic_str = unsafe {
                str::from_utf8_unchecked(slice::from_raw_parts(
                    topic.data, topic.len,
                ))
            };

            let unpacked = match unsafe { c::gg_obj_type(payload) } {
                c::GgObjectType::GG_TYPE_MAP => {
                    let map = unsafe { c::gg_obj_into_map(payload) };
                    SubscribeToTopicPayload::Json(MapRef(unsafe {
                        slice::from_raw_parts(
                            map.pairs as *const KvRef,
                            map.len,
                        )
                    }))
                }
                c::GgObjectType::GG_TYPE_BUF => {
                    let buf = unsafe { c::gg_obj_into_buf(payload) };
                    SubscribeToTopicPayload::Binary(unsafe {
                        slice::from_raw_parts(buf.data, buf.len)
                    })
                }
                _ => return,
            };

            cb(topic_str, unpacked);
        }

        let topic = topic.into();
        let topic_buf = c::GgBuffer {
            data: topic.as_ptr().cast_mut(),
            len: topic.len(),
        };

        let ctx = Box::into_raw(Box::new(callback));
        let mut handle = c::GgIpcSubscriptionHandle { val: 0 };

        Result::from(unsafe {
            c::ggipc_subscribe_to_topic(
                topic_buf,
                Some(trampoline::<F>),
                ctx.cast::<ffi::c_void>(),
                &raw mut handle,
            )
        })
        .inspect_err(|_| unsafe {
            drop(Box::from_raw(ctx));
        })?;

        debug_assert!(handle.val != 0);
        Ok(Subscription {
            handle,
            ctx,
            phantom: PhantomData,
        })
    }

    /// Publish an MQTT message to AWS IoT Core.
    ///
    /// Sends messages to AWS IoT Core MQTT broker with specified QoS.
    /// Requires `aws.greengrass#PublishToIoTCore` authorization.
    ///
    /// See: <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-iot-core-mqtt.html#ipc-operation-publishtoiotcore>
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use gg_sdk::{Sdk, Qos};
    ///
    /// let sdk = Sdk::init();
    /// sdk.connect()?;
    ///
    /// let payload = b"telemetry data";
    /// sdk.publish_to_iot_core("device/telemetry", payload, Qos::AtMostOnce)?;
    /// # Ok::<(), gg_sdk::Error>(())
    /// ```
    ///
    /// # Errors
    /// Returns error if publish fails.
    pub fn publish_to_iot_core<'a>(
        &self,
        topic: impl Into<&'a str>,
        payload: impl AsRef<[u8]>,
        qos: Qos,
    ) -> Result<()> {
        fn inner(topic: &str, payload: &[u8], qos: Qos) -> Result<()> {
            let topic_buf = c::GgBuffer {
                data: topic.as_ptr().cast_mut(),
                len: topic.len(),
            };
            let payload_buf = c::GgBuffer {
                data: payload.as_ptr().cast_mut(),
                len: payload.len(),
            };

            Result::from(unsafe {
                c::ggipc_publish_to_iot_core(topic_buf, payload_buf, qos as u8)
            })
        }
        inner(topic.into(), payload.as_ref(), qos)
    }

    /// Subscribe to MQTT messages from AWS IoT Core.
    ///
    /// Receives messages from AWS IoT Core MQTT broker on matching topics.
    /// Requires `aws.greengrass#SubscribeToIoTCore` authorization.
    ///
    /// See: <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-iot-core-mqtt.html#ipc-operation-subscribetoiotcore>
    ///
    /// # Errors
    /// Returns error if subscription fails.
    pub fn subscribe_to_iot_core<'a, F: FnMut(&str, &[u8]) + 'a>(
        &self,
        topic_filter: impl Into<&'a str>,
        qos: Qos,
        callback: F,
    ) -> Result<Subscription<'a, F>> {
        extern "C" fn trampoline<F: FnMut(&str, &[u8])>(
            ctx: *mut ffi::c_void,
            topic: c::GgBuffer,
            payload: c::GgBuffer,
            _handle: c::GgIpcSubscriptionHandle,
        ) {
            let cb = unsafe { &mut *ctx.cast::<F>() };
            let topic_str = str::from_utf8(unsafe {
                slice::from_raw_parts(topic.data, topic.len)
            })
            .unwrap();
            let payload_bytes =
                unsafe { slice::from_raw_parts(payload.data, payload.len) };
            cb(topic_str, payload_bytes);
        }

        let topic_filter = topic_filter.into();
        let topic_buf = c::GgBuffer {
            data: topic_filter.as_ptr().cast_mut(),
            len: topic_filter.len(),
        };

        let ctx = Box::into_raw(Box::new(callback));
        let mut handle = c::GgIpcSubscriptionHandle { val: 0 };

        Result::from(unsafe {
            c::ggipc_subscribe_to_iot_core(
                topic_buf,
                qos as u8,
                Some(trampoline::<F>),
                ctx.cast::<ffi::c_void>(),
                &raw mut handle,
            )
        })
        .inspect_err(|_| unsafe {
            drop(Box::from_raw(ctx));
        })?;

        debug_assert!(handle.val != 0);
        Ok(Subscription {
            handle,
            ctx,
            phantom: PhantomData,
        })
    }

    /// Get component configuration value.
    ///
    /// Retrieves configuration for the specified key path. Pass empty slice for complete config.
    /// Requires `aws.greengrass#GetConfiguration` authorization.
    ///
    /// See: <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-component-configuration.html#ipc-operation-getconfiguration>
    ///
    /// # Errors
    /// Returns error if config retrieval fails.
    pub fn get_config<'a>(
        &self,
        key_path: &[&str],
        component_name: Option<&str>,
        result_mem: &'a mut [mem::MaybeUninit<u8>],
    ) -> Result<ObjectRef<'a>> {
        let bufs: Box<[c::GgBuffer]> = key_path
            .iter()
            .map(|k| c::GgBuffer {
                data: k.as_ptr().cast_mut(),
                len: k.len(),
            })
            .collect();

        let key_path_list = c::GgBufList {
            bufs: bufs.as_ptr().cast_mut(),
            len: bufs.len(),
        };

        let component_buf = component_name.map(|name| c::GgBuffer {
            data: name.as_ptr().cast_mut(),
            len: name.len(),
        });

        let mut arena = unsafe {
            c::gg_arena_init(c::GgBuffer {
                data: result_mem.as_mut_ptr().cast::<u8>(),
                len: result_mem.len(),
            })
        };

        let mut obj = c::GG_OBJ_NULL;

        Result::from(unsafe {
            c::ggipc_get_config(
                key_path_list,
                component_buf.as_ref().map_or(ptr::null(), ptr::from_ref),
                &raw mut arena,
                &raw mut obj,
            )
        })?;

        Ok(unsafe { ptr::read((&raw const obj).cast()) })
    }

    /// Get component configuration value as a string.
    ///
    /// Retrieves string configuration for the specified key path.
    /// Requires `aws.greengrass#GetConfiguration` authorization.
    ///
    /// See: <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-component-configuration.html#ipc-operation-getconfiguration>
    ///
    /// # Errors
    /// Returns error if config retrieval fails.
    ///
    /// # Panics
    /// Panics if the config value is not valid UTF-8.
    pub fn get_config_str<'a>(
        &self,
        key_path: &[&str],
        component_name: Option<&str>,
        value_buf: &'a mut [mem::MaybeUninit<u8>],
    ) -> Result<&'a str> {
        let bufs: Box<[c::GgBuffer]> = key_path
            .iter()
            .map(|k| c::GgBuffer {
                data: k.as_ptr().cast_mut(),
                len: k.len(),
            })
            .collect();

        let key_path_list = c::GgBufList {
            bufs: bufs.as_ptr().cast_mut(),
            len: bufs.len(),
        };

        let component_buf = component_name.map(|name| c::GgBuffer {
            data: name.as_ptr().cast_mut(),
            len: name.len(),
        });

        let mut value = c::GgBuffer {
            data: value_buf.as_mut_ptr().cast::<u8>(),
            len: value_buf.len(),
        };

        Result::from(unsafe {
            c::ggipc_get_config_str(
                key_path_list,
                component_buf.as_ref().map_or(ptr::null(), ptr::from_ref),
                &raw mut value,
            )
        })?;

        Ok(str::from_utf8(unsafe {
            slice::from_raw_parts(value.data, value.len)
        })
        .unwrap())
    }

    /// Update component configuration.
    ///
    /// Merges the provided value into the component's configuration at the key path.
    /// Requires `aws.greengrass#UpdateConfiguration` authorization.
    ///
    /// See: <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-component-configuration.html#ipc-operation-updateconfiguration>
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use gg_sdk::Sdk;
    ///
    /// let sdk = Sdk::init();
    /// sdk.connect()?;
    ///
    /// sdk.update_config(&["maxRetries"], None, 100_i64)?;
    /// # Ok::<(), gg_sdk::Error>(())
    /// ```
    ///
    /// # Errors
    /// Returns error if config update fails.
    pub fn update_config<'a>(
        &self,
        key_path: &[&str],
        timestamp: Option<std::time::SystemTime>,
        value_to_merge: impl Into<ObjectRef<'a>>,
    ) -> Result<()> {
        let value_to_merge = value_to_merge.into();
        let bufs: Box<[c::GgBuffer]> = key_path
            .iter()
            .map(|k| c::GgBuffer {
                data: k.as_ptr().cast_mut(),
                len: k.len(),
            })
            .collect();

        let key_path_list = c::GgBufList {
            bufs: bufs.as_ptr().cast_mut(),
            len: bufs.len(),
        };

        let timespec = timestamp.map(|t| {
            let duration =
                t.duration_since(std::time::UNIX_EPOCH).unwrap_or_default();
            #[expect(clippy::cast_possible_wrap)]
            c::timespec {
                tv_sec: duration.as_secs() as i64,
                tv_nsec: i64::from(duration.subsec_nanos()),
            }
        });

        Result::from(unsafe {
            c::ggipc_update_config(
                key_path_list,
                timespec.as_ref().map_or(ptr::null(), ptr::from_ref),
                *ptr::from_ref(&value_to_merge).cast::<c::GgObject>(),
            )
        })
    }

    /// Update component state.
    ///
    /// Reports component state to the Greengrass nucleus.
    ///
    /// See: <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-component-lifecycle.html#ipc-operation-updatestate>
    ///
    /// # Errors
    /// Returns error if state update fails.
    pub fn update_state(&self, state: ComponentState) -> Result<()> {
        let c_state = match state {
            ComponentState::Running => {
                c::GgComponentState::GG_COMPONENT_STATE_RUNNING
            }
            ComponentState::Errored => {
                c::GgComponentState::GG_COMPONENT_STATE_ERRORED
            }
        };
        Result::from(unsafe { c::ggipc_update_state(c_state) })
    }

    /// Restart a Greengrass component.
    ///
    /// Requests the nucleus to restart the specified component.
    /// Requires appropriate lifecycle management authorization.
    ///
    /// See: <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-component-lifecycle.html>
    ///
    /// # Examples
    ///
    /// ```no_run
    /// use gg_sdk::Sdk;
    ///
    /// let sdk = Sdk::init();
    /// sdk.connect()?;
    ///
    /// sdk.restart_component("com.example.MyComponent")?;
    /// # Ok::<(), gg_sdk::Error>(())
    /// ```
    ///
    /// # Errors
    /// Returns error if restart fails.
    pub fn restart_component<'a>(
        &self,
        component_name: impl Into<&'a str>,
    ) -> Result<()> {
        fn inner(component_name: &str) -> Result<()> {
            let component_buf = c::GgBuffer {
                data: component_name.as_ptr().cast_mut(),
                len: component_name.len(),
            };

            Result::from(unsafe { c::ggipc_restart_component(component_buf) })
        }
        inner(component_name.into())
    }

    /// Subscribe to component configuration updates.
    ///
    /// Receives notifications when configuration changes for the specified key path.
    /// Requires `aws.greengrass#SubscribeToConfigurationUpdate` authorization.
    ///
    /// See: <https://docs.aws.amazon.com/greengrass/v2/developerguide/ipc-component-configuration.html#ipc-operation-subscribetoconfigurationupdate>
    ///
    /// # Errors
    /// Returns error if subscription fails.
    pub fn subscribe_to_configuration_update<
        'a,
        F: FnMut(&str, &[&str]) + 'a,
    >(
        &self,
        component_name: Option<&str>,
        key_path: &[&str],
        callback: F,
    ) -> Result<Subscription<'a, F>> {
        extern "C" fn trampoline<F: FnMut(&str, &[&str])>(
            ctx: *mut ffi::c_void,
            component_name: c::GgBuffer,
            key_path: c::GgList,
            _handle: c::GgIpcSubscriptionHandle,
        ) {
            let cb = unsafe { &mut *ctx.cast::<F>() };
            let component_str = str::from_utf8(unsafe {
                slice::from_raw_parts(component_name.data, component_name.len)
            })
            .unwrap();
            let path_objs =
                unsafe { slice::from_raw_parts(key_path.items, key_path.len) };
            let path_strs: Box<[&str]> = path_objs
                .iter()
                .map(|obj| {
                    let buf = unsafe { c::gg_obj_into_buf(*obj) };
                    str::from_utf8(unsafe {
                        slice::from_raw_parts(buf.data, buf.len)
                    })
                    .unwrap()
                })
                .collect();

            cb(component_str, &path_strs);
        }

        let bufs: Box<[c::GgBuffer]> = key_path
            .iter()
            .map(|k| c::GgBuffer {
                data: k.as_ptr().cast_mut(),
                len: k.len(),
            })
            .collect();

        let key_path_list = c::GgBufList {
            bufs: bufs.as_ptr().cast_mut(),
            len: bufs.len(),
        };

        let component_buf = component_name.map(|name| c::GgBuffer {
            data: name.as_ptr().cast_mut(),
            len: name.len(),
        });

        let ctx = Box::into_raw(Box::new(callback));
        let mut handle = c::GgIpcSubscriptionHandle { val: 0 };

        Result::from(unsafe {
            c::ggipc_subscribe_to_configuration_update(
                component_buf.as_ref().map_or(ptr::null(), ptr::from_ref),
                key_path_list,
                Some(trampoline::<F>),
                ctx.cast::<ffi::c_void>(),
                &raw mut handle,
            )
        })
        .inspect_err(|_| unsafe {
            drop(Box::from_raw(ctx));
        })?;

        debug_assert!(handle.val != 0);
        Ok(Subscription {
            handle,
            ctx,
            phantom: PhantomData,
        })
    }

    /// Make a generic IPC call.
    ///
    /// Low-level interface for invoking IPC operations not covered by specific methods.
    ///
    /// # Errors
    /// Returns error if IPC call fails.
    pub fn call<
        'a,
        'b,
        F: FnOnce(result::Result<&'b [KvRef<'b>], IpcError<'b>>) -> Result<()>,
    >(
        &self,
        operation: &str,
        service_model_type: &str,
        params: &[KvRef<'a>],
        mut callback: F,
    ) -> Result<()> {
        extern "C" fn result_trampoline<
            'b,
            F: FnOnce(result::Result<&'b [KvRef<'b>], IpcError<'b>>) -> Result<()>,
        >(
            ctx: *mut ffi::c_void,
            result: c::GgMap,
        ) -> c::GgError {
            let cb = unsafe { ctx.cast::<F>().read() };
            let result_slice = unsafe {
                slice::from_raw_parts(result.pairs as *const KvRef, result.len)
            };
            cb(Ok(result_slice)).into()
        }

        extern "C" fn error_trampoline<
            'b,
            F: FnOnce(result::Result<&'b [KvRef<'b>], IpcError<'b>>) -> Result<()>,
        >(
            ctx: *mut ffi::c_void,
            error_code: c::GgBuffer,
            message: c::GgBuffer,
        ) -> c::GgError {
            let cb = unsafe { ctx.cast::<F>().read() };
            let code = str::from_utf8(unsafe {
                slice::from_raw_parts(error_code.data, error_code.len)
            })
            .unwrap();
            let msg = str::from_utf8(unsafe {
                slice::from_raw_parts(message.data, message.len)
            })
            .unwrap();
            cb(Err(IpcError {
                error_code: code,
                message: msg,
            }))
            .into()
        }

        let operation_buf = c::GgBuffer {
            data: operation.as_ptr().cast_mut(),
            len: operation.len(),
        };
        let service_model_type_buf = c::GgBuffer {
            data: service_model_type.as_ptr().cast_mut(),
            len: service_model_type.len(),
        };
        let params_map = c::GgMap {
            pairs: params.as_ptr() as *mut c::GgKV,
            len: params.len(),
        };

        Result::from(unsafe {
            c::ggipc_call(
                operation_buf,
                service_model_type_buf,
                params_map,
                Some(result_trampoline::<F>),
                Some(error_trampoline::<F>),
                (&raw mut callback).cast::<ffi::c_void>(),
            )
        })
    }

    /// Subscribe to a generic IPC stream.
    ///
    /// Low-level interface for subscribing to IPC operations not covered by specific methods.
    ///
    /// # Errors
    /// Returns error if subscription fails.
    pub fn subscribe<
        'a,
        'b,
        'c,
        F: FnOnce(result::Result<&'b [KvRef<'b>], IpcError<'b>>) -> Result<()>,
        G: FnMut(usize, &str, &'b [KvRef<'b>]) -> Result<()> + 'c,
    >(
        &self,
        operation: &str,
        service_model_type: &str,
        params: &[KvRef<'a>],
        mut response_callback: F,
        sub_callback: G,
        aux_ctx: usize,
    ) -> Result<Subscription<'c, G>> {
        extern "C" fn result_trampoline<
            'b,
            F: FnOnce(result::Result<&'b [KvRef<'b>], IpcError<'b>>) -> Result<()>,
        >(
            ctx: *mut ffi::c_void,
            result: c::GgMap,
        ) -> c::GgError {
            let cb = unsafe { ctx.cast::<F>().read() };
            let result_slice = unsafe {
                slice::from_raw_parts(result.pairs as *const KvRef, result.len)
            };
            cb(Ok(result_slice)).into()
        }

        extern "C" fn error_trampoline<
            'b,
            F: FnOnce(result::Result<&'b [KvRef<'b>], IpcError<'b>>) -> Result<()>,
        >(
            ctx: *mut ffi::c_void,
            error_code: c::GgBuffer,
            message: c::GgBuffer,
        ) -> c::GgError {
            let cb = unsafe { ctx.cast::<F>().read() };
            let code = str::from_utf8(unsafe {
                slice::from_raw_parts(error_code.data, error_code.len)
            })
            .unwrap();
            let msg = str::from_utf8(unsafe {
                slice::from_raw_parts(message.data, message.len)
            })
            .unwrap();
            cb(Err(IpcError {
                error_code: code,
                message: msg,
            }))
            .into()
        }

        extern "C" fn sub_trampoline<
            'b,
            'c,
            G: FnMut(usize, &str, &'b [KvRef<'b>]) -> Result<()> + 'c,
        >(
            ctx: *mut ffi::c_void,
            aux_ctx: *mut ffi::c_void,
            _handle: c::GgIpcSubscriptionHandle,
            service_model_type: c::GgBuffer,
            data: c::GgMap,
        ) -> c::GgError {
            let cb = unsafe { &mut *ctx.cast::<G>() };
            let aux = aux_ctx as usize;
            let smt = str::from_utf8(unsafe {
                slice::from_raw_parts(
                    service_model_type.data,
                    service_model_type.len,
                )
            })
            .unwrap();
            let map = unsafe {
                slice::from_raw_parts(data.pairs.cast::<KvRef>(), data.len)
            };
            cb(aux, smt, map).into()
        }

        let operation_buf = c::GgBuffer {
            data: operation.as_ptr().cast_mut(),
            len: operation.len(),
        };
        let service_model_type_buf = c::GgBuffer {
            data: service_model_type.as_ptr().cast_mut(),
            len: service_model_type.len(),
        };
        let params_map = c::GgMap {
            pairs: params.as_ptr() as *mut c::GgKV,
            len: params.len(),
        };

        let mut handle = c::GgIpcSubscriptionHandle { val: 0 };
        let ctx = Box::into_raw(Box::new(sub_callback));

        Result::from(unsafe {
            c::ggipc_subscribe(
                operation_buf,
                service_model_type_buf,
                params_map,
                Some(result_trampoline::<F>),
                Some(error_trampoline::<F>),
                (&raw mut response_callback).cast::<ffi::c_void>(),
                Some(sub_trampoline::<'b, 'c, G>),
                ctx.cast::<ffi::c_void>(),
                aux_ctx as *mut ffi::c_void,
                &raw mut handle,
            )
        })
        .inspect_err(|_| unsafe {
            drop(Box::from_raw(ctx));
        })?;

        Ok(Subscription {
            handle,
            ctx,
            phantom: PhantomData,
        })
    }
}

/// Handle for an active IPC subscription.
#[derive(Debug)]
pub struct Subscription<'a, T> {
    handle: c::GgIpcSubscriptionHandle,
    ctx: *mut T,
    phantom: PhantomData<&'a mut T>,
}

impl<T> Drop for Subscription<'_, T> {
    fn drop(&mut self) {
        if self.handle.val != 0 {
            unsafe { c::ggipc_close_subscription(self.handle) };
            if !self.ctx.is_null() {
                let _ = unsafe { Box::from_raw(self.ctx) };
            }
        }
    }
}

impl<T> Default for Subscription<'_, T> {
    fn default() -> Self {
        Self {
            handle: c::GgIpcSubscriptionHandle { val: 0 },
            ctx: ptr::null_mut(),
            phantom: PhantomData,
        }
    }
}
