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
    object::{self, Kv, Mutable, Object, RefKind, Shared},
};

static INIT: OnceLock<()> = OnceLock::new();
static CONNECTED: Mutex<bool> = Mutex::new(false);

#[non_exhaustive]
#[derive(Debug, Clone, Copy)]
pub struct Sdk {}

#[derive(Debug, Clone, Copy)]
pub struct IpcError<'a> {
    pub error_code: &'a str,
    pub message: &'a str,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Qos {
    AtMostOnce = 0,
    AtLeastOnce = 1,
}

#[derive(Debug, Clone, Copy)]
pub enum SubscribeToTopicPayload<'a> {
    Json(&'a [Kv<'a, Shared>]),
    Binary(&'a [u8]),
}

impl Sdk {
    pub fn init() -> Self {
        INIT.get_or_init(|| unsafe { c::ggl_sdk_init() });
        Self {}
    }

    /// # Errors
    /// Returns error if environment variables are missing, already connected, or connection fails.
    pub fn connect(&self) -> Result<()> {
        let svcuid = env::var("SVCUID").map_err(|_| Error::Config)?;
        let socket_path =
            env::var("AWS_GG_NUCLEUS_DOMAIN_SOCKET_FILEPATH_FOR_COMPONENT")
                .map_err(|_| Error::Config)?;

        self.connect_with_token(&socket_path, &svcuid)
    }

    /// # Errors
    /// Returns error if already connected or connection fails.
    #[allow(clippy::missing_panics_doc)]
    pub fn connect_with_token(
        &self,
        socket_path: &str,
        auth_token: &str,
    ) -> Result<()> {
        let mut connected = CONNECTED.lock().unwrap();
        if *connected {
            return Err(Error::Failure);
        }

        let socket_buf = c::GglBuffer {
            data: socket_path.as_ptr().cast_mut(),
            len: socket_path.len(),
        };
        let token_buf = c::GglBuffer {
            data: auth_token.as_ptr().cast_mut(),
            len: auth_token.len(),
        };

        Result::from(unsafe {
            c::ggipc_connect_with_token(socket_buf, token_buf)
        })?;

        *connected = true;
        Ok(())
    }

    /// # Errors
    /// Returns error if publish fails.
    pub fn publish_to_topic_json<'a, R: RefKind<'a>>(
        &self,
        topic: &str,
        payload: &[Kv<'a, R>],
    ) -> Result<()> {
        let topic_buf = c::GglBuffer {
            data: topic.as_ptr().cast_mut(),
            len: topic.len(),
        };
        let payload_map = c::GglMap {
            pairs: payload.as_ptr() as *mut c::GglKV,
            len: payload.len(),
        };

        Result::from(unsafe {
            c::ggipc_publish_to_topic_json(topic_buf, payload_map)
        })
    }

    /// # Errors
    /// Returns error if publish fails.
    pub fn publish_to_topic_binary(
        &self,
        topic: &str,
        payload: &[u8],
    ) -> Result<()> {
        let topic_buf = c::GglBuffer {
            data: topic.as_ptr().cast_mut(),
            len: topic.len(),
        };
        let payload_buf = c::GglBuffer {
            data: payload.as_ptr().cast_mut(),
            len: payload.len(),
        };

        Result::from(unsafe {
            c::ggipc_publish_to_topic_binary(topic_buf, payload_buf)
        })
    }

    /// # Errors
    /// Returns error if subscription fails.
    pub fn subscribe_to_topic<
        'a,
        F: FnMut(&str, SubscribeToTopicPayload) + 'a,
    >(
        &self,
        topic: &str,
        callback: F,
    ) -> Result<Subscription<'a, F>> {
        extern "C" fn trampoline<F: FnMut(&str, SubscribeToTopicPayload)>(
            ctx: *mut ffi::c_void,
            topic: c::GglBuffer,
            payload: c::GglObject,
            _handle: c::GgIpcSubscriptionHandle,
        ) {
            let cb = unsafe { &mut *ctx.cast::<F>() };
            let topic_str = unsafe {
                str::from_utf8_unchecked(slice::from_raw_parts(
                    topic.data, topic.len,
                ))
            };

            let unpacked = match unsafe { c::ggl_obj_type(payload) } {
                c::GglObjectType::GGL_TYPE_MAP => {
                    let map = unsafe { c::ggl_obj_into_map(payload) };
                    SubscribeToTopicPayload::Json(unsafe {
                        slice::from_raw_parts(
                            map.pairs as *const Kv<Shared>,
                            map.len,
                        )
                    })
                }
                c::GglObjectType::GGL_TYPE_BUF => {
                    let buf = unsafe { c::ggl_obj_into_buf(payload) };
                    SubscribeToTopicPayload::Binary(unsafe {
                        slice::from_raw_parts(buf.data, buf.len)
                    })
                }
                _ => return,
            };

            cb(topic_str, unpacked);
        }

        let topic_buf = c::GglBuffer {
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

    /// # Errors
    /// Returns error if publish fails.
    pub fn publish_to_iot_core(
        &self,
        topic: &str,
        payload: &[u8],
        qos: Qos,
    ) -> Result<()> {
        let topic_buf = c::GglBuffer {
            data: topic.as_ptr().cast_mut(),
            len: topic.len(),
        };
        let payload_buf = c::GglBuffer {
            data: payload.as_ptr().cast_mut(),
            len: payload.len(),
        };

        Result::from(unsafe {
            c::ggipc_publish_to_iot_core(topic_buf, payload_buf, qos as u8)
        })
    }

    /// # Errors
    /// Returns error if subscription fails.
    pub fn subscribe_to_iot_core<'a, F: FnMut(&str, &[u8]) + 'a>(
        &self,
        topic_filter: &str,
        qos: Qos,
        callback: F,
    ) -> Result<Subscription<'a, F>> {
        extern "C" fn trampoline<F: FnMut(&str, &[u8])>(
            ctx: *mut ffi::c_void,
            topic: c::GglBuffer,
            payload: c::GglBuffer,
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

        let topic_buf = c::GglBuffer {
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

    /// # Errors
    /// Returns error if config retrieval fails.
    pub fn get_config<'a>(
        &self,
        key_path: &[&str],
        component_name: Option<&str>,
        result_mem: &'a mut [mem::MaybeUninit<u8>],
    ) -> Result<Object<'a, Mutable>> {
        let bufs: Box<[c::GglBuffer]> = key_path
            .iter()
            .map(|k| c::GglBuffer {
                data: k.as_ptr().cast_mut(),
                len: k.len(),
            })
            .collect();

        let key_path_list = c::GglBufList {
            bufs: bufs.as_ptr().cast_mut(),
            len: bufs.len(),
        };

        let component_buf = component_name.map(|name| c::GglBuffer {
            data: name.as_ptr().cast_mut(),
            len: name.len(),
        });

        let mut arena = unsafe {
            c::ggl_arena_init(c::GglBuffer {
                data: result_mem.as_mut_ptr().cast::<u8>(),
                len: result_mem.len(),
            })
        };

        let mut obj = c::GGL_OBJ_NULL;

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
        let bufs: Box<[c::GglBuffer]> = key_path
            .iter()
            .map(|k| c::GglBuffer {
                data: k.as_ptr().cast_mut(),
                len: k.len(),
            })
            .collect();

        let key_path_list = c::GglBufList {
            bufs: bufs.as_ptr().cast_mut(),
            len: bufs.len(),
        };

        let component_buf = component_name.map(|name| c::GglBuffer {
            data: name.as_ptr().cast_mut(),
            len: name.len(),
        });

        let mut value = c::GglBuffer {
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

    /// # Errors
    /// Returns error if config update fails.
    pub fn update_config<'a, R: RefKind<'a>>(
        &self,
        key_path: &[&str],
        timestamp: Option<std::time::SystemTime>,
        value_to_merge: &object::Object<'a, R>,
    ) -> Result<()> {
        let bufs: Box<[c::GglBuffer]> = key_path
            .iter()
            .map(|k| c::GglBuffer {
                data: k.as_ptr().cast_mut(),
                len: k.len(),
            })
            .collect();

        let key_path_list = c::GglBufList {
            bufs: bufs.as_ptr().cast_mut(),
            len: bufs.len(),
        };

        let timespec = timestamp.map(|t| {
            let duration =
                t.duration_since(std::time::UNIX_EPOCH).unwrap_or_default();
            #[allow(clippy::cast_possible_wrap)]
            c::timespec {
                tv_sec: duration.as_secs() as i64,
                tv_nsec: i64::from(duration.subsec_nanos()),
            }
        });

        Result::from(unsafe {
            c::ggipc_update_config(
                key_path_list,
                timespec.as_ref().map_or(ptr::null(), ptr::from_ref),
                *ptr::from_ref(value_to_merge).cast::<c::GglObject>(),
            )
        })
    }

    /// # Errors
    /// Returns error if restart fails.
    pub fn restart_component(&self, component_name: &str) -> Result<()> {
        let component_buf = c::GglBuffer {
            data: component_name.as_ptr().cast_mut(),
            len: component_name.len(),
        };

        Result::from(unsafe { c::ggipc_restart_component(component_buf) })
    }

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
            component_name: c::GglBuffer,
            key_path: c::GglList,
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
                    let buf = unsafe { c::ggl_obj_into_buf(*obj) };
                    str::from_utf8(unsafe {
                        slice::from_raw_parts(buf.data, buf.len)
                    })
                    .unwrap()
                })
                .collect();

            cb(component_str, &path_strs);
        }

        let bufs: Box<[c::GglBuffer]> = key_path
            .iter()
            .map(|k| c::GglBuffer {
                data: k.as_ptr().cast_mut(),
                len: k.len(),
            })
            .collect();

        let key_path_list = c::GglBufList {
            bufs: bufs.as_ptr().cast_mut(),
            len: bufs.len(),
        };

        let component_buf = component_name.map(|name| c::GglBuffer {
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

    /// # Errors
    /// Returns error if IPC call fails.
    pub fn call<
        'a,
        'b,
        R: RefKind<'a>,
        F: FnOnce(
            result::Result<&'b [Kv<'b, Shared>], IpcError<'b>>,
        ) -> Result<()>,
    >(
        &self,
        operation: &str,
        service_model_type: &str,
        params: &[Kv<'a, R>],
        mut callback: F,
    ) -> Result<()> {
        extern "C" fn result_trampoline<
            'b,
            F: FnOnce(
                result::Result<&'b [Kv<'b, Shared>], IpcError<'b>>,
            ) -> Result<()>,
        >(
            ctx: *mut ffi::c_void,
            result: c::GglMap,
        ) -> c::GglError {
            let cb = unsafe { ctx.cast::<F>().read() };
            let result_slice = unsafe {
                slice::from_raw_parts(
                    result.pairs as *const Kv<Shared>,
                    result.len,
                )
            };
            cb(Ok(result_slice)).into()
        }

        extern "C" fn error_trampoline<
            'b,
            F: FnOnce(
                result::Result<&'b [Kv<'b, Shared>], IpcError<'b>>,
            ) -> Result<()>,
        >(
            ctx: *mut ffi::c_void,
            error_code: c::GglBuffer,
            message: c::GglBuffer,
        ) -> c::GglError {
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

        let operation_buf = c::GglBuffer {
            data: operation.as_ptr().cast_mut(),
            len: operation.len(),
        };
        let service_model_type_buf = c::GglBuffer {
            data: service_model_type.as_ptr().cast_mut(),
            len: service_model_type.len(),
        };
        let params_map = c::GglMap {
            pairs: params.as_ptr() as *mut c::GglKV,
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

    /// # Errors
    /// Returns error if subscription fails.
    #[allow(clippy::too_many_lines)]
    pub fn subscribe<
        'a,
        'b,
        'c,
        R: RefKind<'a>,
        F: FnOnce(
            result::Result<&'b [Kv<'b, Shared>], IpcError<'b>>,
        ) -> Result<()>,
        G: FnMut(usize, &str, &'b [Kv<'b, Shared>]) -> Result<()> + 'c,
    >(
        &self,
        operation: &str,
        service_model_type: &str,
        params: &[Kv<'a, R>],
        mut response_callback: F,
        sub_callback: G,
        aux_ctx: usize,
    ) -> Result<Subscription<'c, G>> {
        extern "C" fn result_trampoline<
            'b,
            F: FnOnce(
                result::Result<&'b [Kv<'b, Shared>], IpcError<'b>>,
            ) -> Result<()>,
        >(
            ctx: *mut ffi::c_void,
            result: c::GglMap,
        ) -> c::GglError {
            let cb = unsafe { ctx.cast::<F>().read() };
            let result_slice = unsafe {
                slice::from_raw_parts(
                    result.pairs as *const Kv<Shared>,
                    result.len,
                )
            };
            cb(Ok(result_slice)).into()
        }

        extern "C" fn error_trampoline<
            'b,
            F: FnOnce(
                result::Result<&'b [Kv<'b, Shared>], IpcError<'b>>,
            ) -> Result<()>,
        >(
            ctx: *mut ffi::c_void,
            error_code: c::GglBuffer,
            message: c::GglBuffer,
        ) -> c::GglError {
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
            G: FnMut(usize, &str, &'b [Kv<'b, Shared>]) -> Result<()> + 'c,
        >(
            ctx: *mut ffi::c_void,
            aux_ctx: *mut ffi::c_void,
            _handle: c::GgIpcSubscriptionHandle,
            service_model_type: c::GglBuffer,
            data: c::GglMap,
        ) -> c::GglError {
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
                slice::from_raw_parts(data.pairs as *const Kv<Shared>, data.len)
            };
            cb(aux, smt, map).into()
        }

        let operation_buf = c::GglBuffer {
            data: operation.as_ptr().cast_mut(),
            len: operation.len(),
        };
        let service_model_type_buf = c::GglBuffer {
            data: service_model_type.as_ptr().cast_mut(),
            len: service_model_type.len(),
        };
        let params_map = c::GglMap {
            pairs: params.as_ptr() as *mut c::GglKV,
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
