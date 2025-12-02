// aws-greengrass-component-sdk - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

use core::{
    fmt, marker::PhantomData, ops::Deref, ptr, ptr::NonNull, slice, str,
};

use crate::c;

/// A generic object.
#[derive(Copy, Clone)]
#[repr(transparent)]
pub struct Object<'a> {
    c: c::GgObject,
    phantom: PhantomData<UnpackedObject<'a>>,
}

impl fmt::Debug for Object<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.unpack().fmt(f)
    }
}

/// Unpacked object value.
#[derive(Debug)]
#[repr(u8)]
pub enum UnpackedObject<'a> {
    /// Null value.
    Null = 0,
    /// Boolean value.
    Bool(bool) = 1,
    /// Signed 64-bit integer.
    I64(i64) = 2,
    /// 64-bit floating point.
    F64(f64) = 3,
    /// UTF-8 string buffer.
    Buf(&'a str) = 4,
    /// List of objects.
    List(List<'a>) = 5,
    /// Map of key-value pairs.
    Map(Map<'a>) = 6,
}

impl<'a> Object<'a> {
    /// Null object constant.
    pub const NULL: Self = Self {
        c: c::GgObject { _private: [0; _] },
        phantom: PhantomData,
    };

    /// Create a boolean reference.
    ///
    /// # Examples
    ///
    /// ```
    /// use gg_sdk::{Object, UnpackedObject};
    ///
    /// let obj = Object::bool(false);
    /// assert!(matches!(obj.unpack(), UnpackedObject::Bool(false)));
    /// ```
    #[must_use]
    pub fn bool(b: bool) -> Self {
        Self {
            c: unsafe { c::gg_obj_bool(b) },
            phantom: PhantomData,
        }
    }

    /// Create a signed integer reference.
    #[must_use]
    pub fn i64(i: i64) -> Self {
        Self {
            c: unsafe { c::gg_obj_i64(i) },
            phantom: PhantomData,
        }
    }

    /// Create a floating point reference.
    #[must_use]
    pub fn f64(f: f64) -> Self {
        Self {
            c: unsafe { c::gg_obj_f64(f) },
            phantom: PhantomData,
        }
    }

    /// Create a string buffer reference.
    ///
    /// # Examples
    ///
    /// ```
    /// use gg_sdk::{Object, UnpackedObject};
    ///
    /// let s = "borrowed";
    /// let obj = Object::buf(s);
    /// assert!(matches!(obj.unpack(), UnpackedObject::Buf("borrowed")));
    /// ```
    #[must_use]
    pub fn buf(buf: impl Into<&'a str>) -> Self {
        let s = buf.into();
        Self {
            c: unsafe {
                c::gg_obj_buf(c::GgBuffer {
                    data: s.as_ptr().cast_mut(),
                    len: s.len(),
                })
            },
            phantom: PhantomData,
        }
    }

    /// Create a list reference.
    ///
    /// # Examples
    ///
    /// ```
    /// use gg_sdk::{Object, UnpackedObject};
    ///
    /// let items = [Object::i64(1), Object::i64(2)];
    /// let obj = Object::list(&items[..]);
    /// if let UnpackedObject::List(list) = obj.unpack() {
    ///     assert_eq!(list.len(), 2);
    /// }
    /// ```
    #[must_use]
    pub fn list(list: impl Into<&'a [Object<'a>]>) -> Self {
        let slice = list.into();
        Self {
            c: unsafe {
                c::gg_obj_list(c::GgList {
                    items: slice.as_ptr().cast_mut().cast::<c::GgObject>(),
                    len: slice.len(),
                })
            },
            phantom: PhantomData,
        }
    }

    /// Create a map reference.
    ///
    /// # Examples
    ///
    /// ```
    /// use gg_sdk::{Kv, Object, UnpackedObject};
    ///
    /// let pairs = [Kv::new("k", Object::i64(1))];
    /// let obj = Object::map(&pairs[..]);
    /// if let UnpackedObject::Map(m) = obj.unpack() {
    ///     assert_eq!(m.len(), 1);
    /// }
    /// ```
    #[must_use]
    pub fn map(map: impl Into<&'a [Kv<'a>]>) -> Self {
        let slice = map.into();
        Self {
            c: unsafe {
                c::gg_obj_map(c::GgMap {
                    pairs: slice.as_ptr().cast_mut().cast::<c::GgKV>(),
                    len: slice.len(),
                })
            },
            phantom: PhantomData,
        }
    }
}

unsafe fn slice_from_c<'a, T>(ptr: *const T, len: usize) -> &'a [T] {
    let ptr = NonNull::new(ptr.cast_mut()).unwrap_or_else(|| {
        assert_eq!(len, 0, "null pointer with non-zero length");
        NonNull::dangling()
    });
    unsafe { NonNull::slice_from_raw_parts(ptr, len).as_ref() }
}

impl<'a> Object<'a> {
    /// Unpack the object to inspect its value.
    ///
    /// # Examples
    ///
    /// ```
    /// use gg_sdk::{Kv, Object, UnpackedObject};
    ///
    /// let obj = Object::i64(42);
    /// match obj.unpack() {
    ///     UnpackedObject::Null => println!("null"),
    ///     UnpackedObject::Bool(b) => println!("bool: {}", b),
    ///     UnpackedObject::I64(n) => assert_eq!(n, 42),
    ///     UnpackedObject::F64(f) => println!("float: {}", f),
    ///     UnpackedObject::Buf(s) => println!("string: {}", s),
    ///     UnpackedObject::List(items) => println!("list of {} items", items.len()),
    ///     UnpackedObject::Map(pairs) => println!("map with {} pairs", pairs.len()),
    /// }
    ///
    /// let items = [Object::i64(1), Object::buf("two")];
    /// let list = Object::list(&items[..]);
    /// if let UnpackedObject::List(items) = list.unpack() {
    ///     assert_eq!(items.len(), 2);
    /// }
    ///
    /// let pairs = [Kv::new("key", Object::bool(true))];
    /// let map = Object::map(&pairs[..]);
    /// if let UnpackedObject::Map(pairs) = map.unpack() {
    ///     assert_eq!(pairs[0].key(), "key");
    /// }
    /// ```
    ///
    /// # Panics
    /// Panics if the buffer is not valid UTF-8.
    #[must_use]
    pub fn unpack(&self) -> UnpackedObject<'_> {
        use c::GgObjectType::*;

        unsafe {
            match c::gg_obj_type(self.c) {
                GG_TYPE_NULL => UnpackedObject::Null,
                GG_TYPE_BOOLEAN => {
                    UnpackedObject::Bool(c::gg_obj_into_bool(self.c))
                }
                GG_TYPE_I64 => UnpackedObject::I64(c::gg_obj_into_i64(self.c)),
                GG_TYPE_F64 => UnpackedObject::F64(c::gg_obj_into_f64(self.c)),
                GG_TYPE_BUF => {
                    let buf = c::gg_obj_into_buf(self.c);
                    let ptr = slice_from_c(buf.data, buf.len);
                    UnpackedObject::Buf(str::from_utf8(ptr).unwrap())
                }
                GG_TYPE_LIST => {
                    let list = c::gg_obj_into_list(self.c);
                    UnpackedObject::List(List(slice_from_c(
                        list.items.cast::<Object<'a>>(),
                        list.len,
                    )))
                }
                GG_TYPE_MAP => {
                    let map = c::gg_obj_into_map(self.c);
                    UnpackedObject::Map(Map(slice_from_c(
                        map.pairs.cast::<Kv<'a>>(),
                        map.len,
                    )))
                }
            }
        }
    }
}

impl Default for Object<'_> {
    fn default() -> Self {
        Self::NULL
    }
}

impl From<bool> for Object<'_> {
    fn from(b: bool) -> Self {
        Self::bool(b)
    }
}

impl From<i64> for Object<'_> {
    fn from(i: i64) -> Self {
        Self::i64(i)
    }
}

impl From<f64> for Object<'_> {
    fn from(f: f64) -> Self {
        Self::f64(f)
    }
}

impl<'a> From<&'a str> for Object<'a> {
    fn from(s: &'a str) -> Self {
        Self::buf(s)
    }
}

impl<'a> From<&'a [Object<'a>]> for Object<'a> {
    fn from(list: &'a [Object<'a>]) -> Self {
        Self::list(list)
    }
}

impl<'a> From<&'a [Kv<'a>]> for Object<'a> {
    fn from(map: &'a [Kv<'a>]) -> Self {
        Self::map(map)
    }
}

impl<'a> From<Map<'a>> for Object<'a> {
    fn from(map: Map<'a>) -> Self {
        Self::map(map.0)
    }
}

impl<'a> From<List<'a>> for Object<'a> {
    fn from(list: List<'a>) -> Self {
        Self::list(list.0)
    }
}

/// A key-value pair.
#[derive(Copy, Clone)]
#[repr(transparent)]
pub struct Kv<'a> {
    c: c::GgKV,
    phantom_key: PhantomData<&'a str>,
    phantom_value: PhantomData<Object<'a>>,
}

impl fmt::Debug for Kv<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Kv")
            .field("key", &self.key())
            .field("val", self.val())
            .finish()
    }
}

impl<'a> Kv<'a> {
    /// Create a key-value pair.
    ///
    /// # Examples
    ///
    /// ```
    /// use gg_sdk::{Kv, Object};
    ///
    /// let kv = Kv::new("key", Object::i64(10));
    /// assert_eq!(kv.key(), "key");
    /// ```
    pub fn new(key: impl Into<&'a str>, value: Object<'a>) -> Self {
        let s = key.into();
        Self {
            c: unsafe {
                c::gg_kv(
                    c::GgBuffer {
                        data: s.as_ptr().cast_mut(),
                        len: s.len(),
                    },
                    value.c,
                )
            },
            phantom_key: PhantomData,
            phantom_value: PhantomData,
        }
    }

    /// # Panics
    /// Panics if the key is not valid UTF-8.
    #[must_use]
    pub fn key(&self) -> &str {
        let buf = unsafe { c::gg_kv_key(self.c) };
        unsafe {
            str::from_utf8(slice::from_raw_parts(buf.data, buf.len)).unwrap()
        }
    }

    /// Get a reference to the value.
    ///
    /// # Examples
    ///
    /// ```
    /// use gg_sdk::{Kv, Object, UnpackedObject};
    ///
    /// let kv = Kv::new("key", Object::i64(100));
    /// assert!(matches!(kv.val().unpack(), UnpackedObject::I64(100)));
    /// ```
    #[must_use]
    pub fn val(&self) -> &Object<'a> {
        unsafe {
            c::gg_kv_val((&raw const self.c).cast_mut())
                .cast::<Object>()
                .as_ref()
                .unwrap_unchecked()
        }
    }
}

impl<'a> From<Map<'a>> for &'a [Kv<'a>] {
    fn from(map: Map<'a>) -> Self {
        map.0
    }
}

impl<'a> From<List<'a>> for &'a [Object<'a>] {
    fn from(list: List<'a>) -> Self {
        list.0
    }
}

/// A map of UTF-8 strings to objects.
#[derive(Debug, Clone, Copy)]
#[repr(transparent)]
pub struct Map<'a>(pub &'a [Kv<'a>]);

/// An array of objects.
#[derive(Debug, Clone, Copy)]
#[repr(transparent)]
pub struct List<'a>(pub &'a [Object<'a>]);

impl Map<'_> {
    /// Get a value from a map by key.
    ///
    /// # Examples
    ///
    /// ```
    /// use gg_sdk::{Kv, Map, Object, UnpackedObject};
    ///
    /// let pairs = [
    ///     Kv::new("a", Object::i64(1)),
    ///     Kv::new("b", Object::i64(2)),
    /// ];
    /// let map: Map = pairs.as_slice().into();
    /// let val = map.get("b").unwrap();
    /// assert!(matches!(val.unpack(), UnpackedObject::I64(2)));
    /// ```
    #[must_use]
    pub fn get<'b>(&self, key: impl Into<&'b str>) -> Option<&Object<'_>> {
        let key = key.into();
        let map = c::GgMap {
            pairs: self.0.as_ptr().cast_mut().cast::<c::GgKV>(),
            len: self.0.len(),
        };
        let key_buf = c::GgBuffer {
            data: key.as_ptr().cast_mut(),
            len: key.len(),
        };
        let mut result: *mut c::GgObject = ptr::null_mut();
        if unsafe { c::gg_map_get(map, key_buf, &raw mut result) } {
            Some(unsafe {
                NonNull::new(result.cast::<Object>())
                    .unwrap_unchecked()
                    .as_ref()
            })
        } else {
            None
        }
    }
}

impl<'a> From<&'a [Kv<'a>]> for Map<'a> {
    fn from(slice: &'a [Kv<'a>]) -> Self {
        Map(slice)
    }
}

impl<'a> From<&'a [Object<'a>]> for List<'a> {
    fn from(slice: &'a [Object<'a>]) -> Self {
        List(slice)
    }
}

impl<'a> Deref for Map<'a> {
    type Target = [Kv<'a>];
    fn deref(&self) -> &[Kv<'a>] {
        self.0
    }
}

impl<'a> Deref for List<'a> {
    type Target = [Object<'a>];
    fn deref(&self) -> &[Object<'a>] {
        self.0
    }
}
