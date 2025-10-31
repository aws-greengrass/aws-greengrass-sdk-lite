// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

use std::{
    borrow::Borrow,
    fmt,
    marker::PhantomData,
    mem::ManuallyDrop,
    ops::Deref,
    ptr::{self, NonNull},
    slice, str,
};

use crate::c;

#[repr(transparent)]
pub struct Object {
    c: c::GglObject,
}

#[repr(transparent)]
pub struct ObjectRef<'a> {
    c: c::GglObject,
    phantom: PhantomData<UnpackedObject<'a>>,
}

impl<'a> AsRef<ObjectRef<'a>> for Object {
    fn as_ref(&self) -> &ObjectRef<'a> {
        self.borrow()
    }
}

impl<'a> Borrow<ObjectRef<'a>> for Object {
    fn borrow(&self) -> &ObjectRef<'a> {
        unsafe {
            (&raw const *self)
                .cast::<ObjectRef>()
                .as_ref()
                .unwrap_unchecked()
        }
    }
}

impl fmt::Debug for Object {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.as_ref().fmt(f)
    }
}

impl fmt::Debug for ObjectRef<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.unpack().fmt(f)
    }
}

#[derive(Debug)]
#[repr(u8)]
pub enum UnpackedObject<'a> {
    Null = 0,
    Bool(bool) = 1,
    I64(i64) = 2,
    F64(f64) = 3,
    Buf(&'a str) = 4,
    List(ListRef<'a>) = 5,
    Map(MapRef<'a>) = 6,
}

impl Object {
    pub const NULL: Self = Self { c: c::GGL_OBJ_NULL };

    /// Create a boolean object.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{Object, UnpackedObject};
    ///
    /// let obj = Object::bool(true);
    /// assert!(matches!(obj.as_ref().unpack(), UnpackedObject::Bool(true)));
    /// ```
    #[must_use]
    pub fn bool(b: bool) -> Self {
        Self {
            c: unsafe { c::ggl_obj_bool(b) },
        }
    }

    /// Create an i64 object.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{Object, UnpackedObject};
    ///
    /// let obj = Object::i64(42);
    /// assert!(matches!(obj.as_ref().unpack(), UnpackedObject::I64(42)));
    /// ```
    #[must_use]
    pub fn i64(i: i64) -> Self {
        Self {
            c: unsafe { c::ggl_obj_i64(i) },
        }
    }

    /// Create an f64 object.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{Object, UnpackedObject};
    ///
    /// let obj = Object::f64(3.14);
    /// assert!(matches!(obj.as_ref().unpack(), UnpackedObject::F64(f) if (f - 3.14).abs() < f64::EPSILON));
    /// ```
    #[must_use]
    pub fn f64(f: f64) -> Self {
        Self {
            c: unsafe { c::ggl_obj_f64(f) },
        }
    }

    /// Create a string buffer object.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{Object, UnpackedObject};
    ///
    /// let obj = Object::buf("hello");
    /// assert!(matches!(obj.as_ref().unpack(), UnpackedObject::Buf("hello")));
    /// ```
    #[must_use]
    pub fn buf(buf: impl Into<Box<str>>) -> Self {
        let s = buf.into();
        let len = s.len();
        let ptr = Box::into_raw(s);
        Self {
            c: unsafe {
                c::ggl_obj_buf(c::GglBuffer {
                    data: (*ptr).as_ptr().cast_mut(),
                    len,
                })
            },
        }
    }

    /// Create a list object.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{Object, UnpackedObject};
    ///
    /// let obj = Object::list([Object::i64(1), Object::i64(2)]);
    /// if let UnpackedObject::List(items) = obj.as_ref().unpack() {
    ///     assert_eq!(items.len(), 2);
    /// }
    /// ```
    #[must_use]
    pub fn list(list: impl Into<Box<[Object]>>) -> Self {
        let ptr = Box::into_raw(list.into());
        Self {
            c: unsafe {
                c::ggl_obj_list(c::GglList {
                    items: (*ptr).as_ptr().cast_mut().cast::<c::GglObject>(),
                    len: ptr.len(),
                })
            },
        }
    }

    /// Create a map object.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{Kv, Object, UnpackedObject};
    ///
    /// let obj = Object::map([
    ///     Kv::new("key1", Object::i64(42)),
    ///     Kv::new("key2", Object::buf("value")),
    /// ]);
    /// if let UnpackedObject::Map(pairs) = obj.as_ref().unpack() {
    ///     assert_eq!(pairs.len(), 2);
    ///     assert!(matches!(
    ///         pairs.get("key1").unwrap().unpack(),
    ///         UnpackedObject::I64(42)
    ///     ));
    /// }
    /// ```
    #[must_use]
    pub fn map(map: impl Into<Box<[Kv]>>) -> Self {
        let ptr = Box::into_raw(map.into());
        Self {
            c: unsafe {
                c::ggl_obj_map(c::GglMap {
                    pairs: (*ptr).as_ptr().cast_mut().cast::<c::GglKV>(),
                    len: ptr.len(),
                })
            },
        }
    }
}

impl<'a> ObjectRef<'a> {
    pub const NULL: Self = Self {
        c: c::GGL_OBJ_NULL,
        phantom: PhantomData,
    };

    /// Create a boolean reference.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{ObjectRef, UnpackedObject};
    ///
    /// let obj = ObjectRef::bool(false);
    /// assert!(matches!(obj.unpack(), UnpackedObject::Bool(false)));
    /// ```
    #[must_use]
    pub fn bool(b: bool) -> Self {
        Self {
            c: unsafe { c::ggl_obj_bool(b) },
            phantom: PhantomData,
        }
    }

    #[must_use]
    pub fn i64(i: i64) -> Self {
        Self {
            c: unsafe { c::ggl_obj_i64(i) },
            phantom: PhantomData,
        }
    }

    #[must_use]
    pub fn f64(f: f64) -> Self {
        Self {
            c: unsafe { c::ggl_obj_f64(f) },
            phantom: PhantomData,
        }
    }

    /// Create a borrowed string buffer reference.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{ObjectRef, UnpackedObject};
    ///
    /// let s = "borrowed";
    /// let obj = ObjectRef::buf(s);
    /// assert!(matches!(obj.unpack(), UnpackedObject::Buf("borrowed")));
    /// ```
    #[must_use]
    pub fn buf(buf: impl Into<&'a str>) -> Self {
        let s = buf.into();
        Self {
            c: unsafe {
                c::ggl_obj_buf(c::GglBuffer {
                    data: s.as_ptr().cast_mut(),
                    len: s.len(),
                })
            },
            phantom: PhantomData,
        }
    }

    /// Create a borrowed list reference.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{ObjectRef, UnpackedObject};
    ///
    /// let items = [ObjectRef::i64(1), ObjectRef::i64(2)];
    /// let obj = ObjectRef::list(&items[..]);
    /// if let UnpackedObject::List(list) = obj.unpack() {
    ///     assert_eq!(list.len(), 2);
    /// }
    /// ```
    #[must_use]
    pub fn list(list: impl Into<&'a [ObjectRef<'a>]>) -> Self {
        let slice = list.into();
        Self {
            c: unsafe {
                c::ggl_obj_list(c::GglList {
                    items: slice.as_ptr().cast_mut().cast::<c::GglObject>(),
                    len: slice.len(),
                })
            },
            phantom: PhantomData,
        }
    }

    /// Create a borrowed map reference.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{KvRef, ObjectRef, UnpackedObject};
    ///
    /// let pairs = [KvRef::new("k", ObjectRef::i64(1))];
    /// let obj = ObjectRef::map(&pairs[..]);
    /// if let UnpackedObject::Map(m) = obj.unpack() {
    ///     assert_eq!(m.len(), 1);
    /// }
    /// ```
    #[must_use]
    pub fn map(map: impl Into<&'a [KvRef<'a>]>) -> Self {
        let slice = map.into();
        Self {
            c: unsafe {
                c::ggl_obj_map(c::GglMap {
                    pairs: slice.as_ptr().cast_mut().cast::<c::GglKV>(),
                    len: slice.len(),
                })
            },
            phantom: PhantomData,
        }
    }
}

impl ToOwned for ObjectRef<'_> {
    type Owned = Object;

    fn to_owned(&self) -> Object {
        use UnpackedObject::*;
        match self.unpack() {
            Null => Object::NULL,
            Bool(b) => Object::bool(b),
            I64(i) => Object::i64(i),
            F64(f) => Object::f64(f),
            Buf(s) => Object::buf(s.to_owned().into_boxed_str()),
            List(items) => {
                let owned: Box<[Object]> =
                    items.iter().map(ObjectRef::to_owned).collect();
                Object::list(owned)
            }
            Map(pairs) => {
                let owned: Box<[Kv]> =
                    pairs.iter().map(KvRef::to_owned).collect();
                Object::map(owned)
            }
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

impl<'a> ObjectRef<'a> {
    /// Unpack the object to inspect its value.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{Kv, Object, UnpackedObject};
    ///
    /// let obj = Object::i64(42);
    /// match obj.as_ref().unpack() {
    ///     UnpackedObject::Null => println!("null"),
    ///     UnpackedObject::Bool(b) => println!("bool: {}", b),
    ///     UnpackedObject::I64(n) => assert_eq!(n, 42),
    ///     UnpackedObject::F64(f) => println!("float: {}", f),
    ///     UnpackedObject::Buf(s) => println!("string: {}", s),
    ///     UnpackedObject::List(items) => println!("list of {} items", items.len()),
    ///     UnpackedObject::Map(pairs) => println!("map with {} pairs", pairs.len()),
    /// }
    ///
    /// let list = Object::list([Object::i64(1), Object::buf("two")]);
    /// if let UnpackedObject::List(items) = list.as_ref().unpack() {
    ///     assert_eq!(items.len(), 2);
    /// }
    ///
    /// let map = Object::map([Kv::new("key", Object::bool(true))]);
    /// if let UnpackedObject::Map(pairs) = map.as_ref().unpack() {
    ///     assert_eq!(pairs[0].key(), "key");
    /// }
    /// ```
    ///
    /// # Panics
    /// Panics if the buffer is not valid UTF-8.
    #[must_use]
    pub fn unpack(&self) -> UnpackedObject<'_> {
        use c::GglObjectType::*;
        use UnpackedObject::*;

        unsafe {
            match c::ggl_obj_type(self.c) {
                GGL_TYPE_NULL => Null,
                GGL_TYPE_BOOLEAN => Bool(c::ggl_obj_into_bool(self.c)),
                GGL_TYPE_I64 => I64(c::ggl_obj_into_i64(self.c)),
                GGL_TYPE_F64 => F64(c::ggl_obj_into_f64(self.c)),
                GGL_TYPE_BUF => {
                    let buf = c::ggl_obj_into_buf(self.c);
                    let ptr = slice_from_c(buf.data, buf.len);
                    Buf(str::from_utf8(ptr).unwrap())
                }
                GGL_TYPE_LIST => {
                    let list = c::ggl_obj_into_list(self.c);
                    List(ListRef(slice_from_c(
                        list.items.cast::<ObjectRef<'a>>(),
                        list.len,
                    )))
                }
                GGL_TYPE_MAP => {
                    let map = c::ggl_obj_into_map(self.c);
                    Map(MapRef(slice_from_c(
                        map.pairs.cast::<KvRef<'a>>(),
                        map.len,
                    )))
                }
            }
        }
    }
}

impl Clone for Object {
    fn clone(&self) -> Self {
        self.as_ref().to_owned()
    }
}

impl Drop for Object {
    fn drop(&mut self) {
        use c::GglObjectType::*;
        unsafe {
            match c::ggl_obj_type(self.c) {
                GGL_TYPE_BUF => {
                    let buf = c::ggl_obj_into_buf(self.c);
                    let _ = Box::from_raw(ptr::slice_from_raw_parts_mut(
                        buf.data, buf.len,
                    ));
                }
                GGL_TYPE_LIST => {
                    let list = c::ggl_obj_into_list(self.c);
                    let _ = Box::from_raw(ptr::slice_from_raw_parts_mut(
                        list.items.cast::<Object>(),
                        list.len,
                    ));
                }
                GGL_TYPE_MAP => {
                    let map = c::ggl_obj_into_map(self.c);
                    let _ = Box::from_raw(ptr::slice_from_raw_parts_mut(
                        map.pairs.cast::<Kv>(),
                        map.len,
                    ));
                }
                _ => {}
            }
        }
    }
}

impl Default for Object {
    fn default() -> Self {
        Self::NULL
    }
}

impl From<bool> for Object {
    fn from(b: bool) -> Self {
        Self::bool(b)
    }
}

impl From<i64> for Object {
    fn from(i: i64) -> Self {
        Self::i64(i)
    }
}

impl From<f64> for Object {
    fn from(f: f64) -> Self {
        Self::f64(f)
    }
}

impl From<Box<str>> for Object {
    fn from(s: Box<str>) -> Self {
        Self::buf(s)
    }
}

impl From<String> for Object {
    fn from(s: String) -> Self {
        Self::buf(s.into_boxed_str())
    }
}

impl From<Box<[Object]>> for Object {
    fn from(list: Box<[Object]>) -> Self {
        Self::list(list)
    }
}

impl From<Box<[Kv]>> for Object {
    fn from(map: Box<[Kv]>) -> Self {
        Self::map(map)
    }
}

impl Default for ObjectRef<'_> {
    fn default() -> Self {
        Self::NULL
    }
}

impl From<bool> for ObjectRef<'_> {
    fn from(b: bool) -> Self {
        Self::bool(b)
    }
}

impl From<i64> for ObjectRef<'_> {
    fn from(i: i64) -> Self {
        Self::i64(i)
    }
}

impl From<f64> for ObjectRef<'_> {
    fn from(f: f64) -> Self {
        Self::f64(f)
    }
}

impl<'a> From<&'a str> for ObjectRef<'a> {
    fn from(s: &'a str) -> Self {
        Self::buf(s)
    }
}

impl<'a> From<&'a [ObjectRef<'a>]> for ObjectRef<'a> {
    fn from(list: &'a [ObjectRef<'a>]) -> Self {
        Self::list(list)
    }
}

impl<'a> From<&'a [KvRef<'a>]> for ObjectRef<'a> {
    fn from(map: &'a [KvRef<'a>]) -> Self {
        Self::map(map)
    }
}

impl<'a> From<&'a ObjectRef<'a>> for ObjectRef<'a> {
    fn from(obj: &'a ObjectRef<'a>) -> Self {
        unsafe { ptr::read(obj) }
    }
}

impl<'a> From<MapRef<'a>> for ObjectRef<'a> {
    fn from(map: MapRef<'a>) -> Self {
        Self::map(map.0)
    }
}

impl<'a> From<ListRef<'a>> for ObjectRef<'a> {
    fn from(list: ListRef<'a>) -> Self {
        Self::list(list.0)
    }
}

impl From<Map> for Object {
    fn from(map: Map) -> Self {
        Self::map(map.0)
    }
}

impl From<List> for Object {
    fn from(list: List) -> Self {
        Self::list(list.0)
    }
}

#[repr(transparent)]
pub struct Kv {
    c: c::GglKV,
}

#[repr(transparent)]
pub struct KvRef<'a> {
    c: c::GglKV,
    phantom_key: PhantomData<&'a str>,
    phantom_value: PhantomData<ObjectRef<'a>>,
}

impl<'a> AsRef<KvRef<'a>> for Kv {
    fn as_ref(&self) -> &KvRef<'a> {
        self.borrow()
    }
}

impl<'a> Borrow<KvRef<'a>> for Kv {
    fn borrow(&self) -> &KvRef<'a> {
        unsafe {
            (&raw const *self)
                .cast::<KvRef>()
                .as_ref()
                .unwrap_unchecked()
        }
    }
}

impl fmt::Debug for Kv {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("KvRef")
            .field("key", &self.key())
            .field("val", self.val())
            .finish()
    }
}

impl fmt::Debug for KvRef<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("KvRef")
            .field("key", &self.key())
            .field("val", self.val())
            .finish()
    }
}

impl Kv {
    /// Create a key-value pair.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{Kv, Object};
    ///
    /// let kv = Kv::new("name", Object::buf("Alice"));
    /// assert_eq!(kv.key(), "name");
    /// ```
    pub fn new(key: impl Into<Box<str>>, value: Object) -> Self {
        let key = key.into();
        let len = key.len();
        let ptr = Box::into_raw(key);
        let value = ManuallyDrop::new(value);
        Self {
            c: unsafe {
                c::ggl_kv(
                    c::GglBuffer {
                        data: (*ptr).as_ptr().cast_mut(),
                        len,
                    },
                    value.c,
                )
            },
        }
    }

    /// # Panics
    /// Panics if the key is not valid UTF-8.
    #[must_use]
    pub fn key(&self) -> &str {
        let buf = unsafe { c::ggl_kv_key(self.c) };
        unsafe {
            str::from_utf8(slice::from_raw_parts(buf.data, buf.len)).unwrap()
        }
    }

    /// Replace the held key.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{Kv, Object};
    ///
    /// let mut kv = Kv::new("old", Object::i64(1));
    /// kv.set_key("new");
    /// assert_eq!(kv.key(), "new");
    /// ```
    pub fn set_key(&mut self, key: impl Into<Box<str>>) {
        let s = key.into();
        let len = s.len();
        let ptr = Box::into_raw(s);
        unsafe {
            let old = c::ggl_kv_key(self.c);
            let _ =
                Box::from_raw(ptr::slice_from_raw_parts_mut(old.data, old.len));
            c::ggl_kv_set_key(
                &raw mut self.c,
                c::GglBuffer {
                    data: (*ptr).as_ptr().cast_mut(),
                    len,
                },
            );
        }
    }

    #[must_use]
    pub fn val(&self) -> &Object {
        unsafe {
            c::ggl_kv_val((&raw const self.c).cast_mut())
                .cast::<Object>()
                .as_ref()
                .unwrap_unchecked()
        }
    }

    /// Get mutable reference to the value.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{Kv, Object, UnpackedObject};
    ///
    /// let mut kv = Kv::new("count", Object::i64(1));
    /// *kv.val_mut() = Object::i64(2);
    /// assert!(matches!(kv.val().as_ref().unpack(), UnpackedObject::I64(2)));
    /// ```
    pub fn val_mut(&mut self) -> &mut Object {
        unsafe {
            c::ggl_kv_val(&raw mut self.c)
                .cast::<Object>()
                .as_mut()
                .unwrap_unchecked()
        }
    }
}

impl<'a> KvRef<'a> {
    /// Create a borrowed key-value pair.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{KvRef, ObjectRef};
    ///
    /// let kv = KvRef::new("key", ObjectRef::i64(10));
    /// assert_eq!(kv.key(), "key");
    /// ```
    #[expect(clippy::needless_pass_by_value)]
    pub fn new(key: impl Into<&'a str>, value: ObjectRef<'a>) -> Self {
        let s = key.into();
        Self {
            c: unsafe {
                c::ggl_kv(
                    c::GglBuffer {
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
        let buf = unsafe { c::ggl_kv_key(self.c) };
        unsafe {
            str::from_utf8(slice::from_raw_parts(buf.data, buf.len)).unwrap()
        }
    }

    /// Get a reference to the value.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{Kv, Object, UnpackedObject};
    ///
    /// let kv = Kv::new("key", Object::i64(100));
    /// assert!(matches!(kv.val().as_ref().unpack(), UnpackedObject::I64(100)));
    /// ```
    #[must_use]
    pub fn val(&self) -> &Object {
        unsafe {
            c::ggl_kv_val((&raw const self.c).cast_mut())
                .cast::<Object>()
                .as_ref()
                .unwrap_unchecked()
        }
    }
}

impl ToOwned for KvRef<'_> {
    type Owned = Kv;

    fn to_owned(&self) -> Kv {
        Kv::new(
            self.key().to_owned().into_boxed_str(),
            self.val().to_owned(),
        )
    }
}

impl Clone for Kv {
    fn clone(&self) -> Self {
        self.as_ref().to_owned()
    }
}

impl Drop for Kv {
    fn drop(&mut self) {
        unsafe {
            let key = c::ggl_kv_key(self.c);
            let _ =
                Box::from_raw(ptr::slice_from_raw_parts_mut(key.data, key.len));
            ptr::drop_in_place(c::ggl_kv_val(&raw mut self.c).cast::<Object>());
        }
    }
}

impl From<Map> for Box<[Kv]> {
    fn from(map: Map) -> Self {
        map.0
    }
}

impl From<List> for Box<[Object]> {
    fn from(list: List) -> Self {
        list.0
    }
}

impl<'a> From<MapRef<'a>> for &'a [KvRef<'a>] {
    fn from(map: MapRef<'a>) -> Self {
        map.0
    }
}

impl<'a> From<ListRef<'a>> for &'a [ObjectRef<'a>] {
    fn from(list: ListRef<'a>) -> Self {
        list.0
    }
}

#[derive(Debug, Clone)]
pub struct Map(pub Box<[Kv]>);

#[derive(Debug, Clone, Copy)]
#[repr(transparent)]
pub struct MapRef<'a>(pub &'a [KvRef<'a>]);

#[derive(Debug, Clone)]
pub struct List(pub Box<[Object]>);

#[derive(Debug, Clone, Copy)]
#[repr(transparent)]
pub struct ListRef<'a>(pub &'a [ObjectRef<'a>]);

impl Map {
    /// Get a value from a map by key.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{Kv, Map, Object, UnpackedObject};
    ///
    /// let map = Map(Box::new([
    ///     Kv::new("a", Object::i64(1)),
    ///     Kv::new("b", Object::i64(2)),
    /// ]));
    /// let val = map.get("b").unwrap();
    /// assert!(matches!(val.unpack(), UnpackedObject::I64(2)));
    /// ```
    #[must_use]
    pub fn get<'b>(&self, key: impl Into<&'b str>) -> Option<&ObjectRef<'_>> {
        self.as_ref().get(key)
    }
}

impl MapRef<'_> {
    /// Get a value from a map by key.
    ///
    /// # Examples
    ///
    /// ```
    /// use ggl_sdk::object::{KvRef, MapRef, ObjectRef, UnpackedObject};
    ///
    /// let pairs = [
    ///     KvRef::new("a", ObjectRef::i64(1)),
    ///     KvRef::new("b", ObjectRef::i64(2)),
    /// ];
    /// let map: MapRef = pairs.as_slice().into();
    /// let val = map.get("b").unwrap();
    /// assert!(matches!(val.unpack(), UnpackedObject::I64(2)));
    /// ```
    #[must_use]
    pub fn get<'b>(&self, key: impl Into<&'b str>) -> Option<&ObjectRef<'_>> {
        let key = key.into();
        let map = c::GglMap {
            pairs: self.0.as_ptr().cast_mut().cast::<c::GglKV>(),
            len: self.0.len(),
        };
        let key_buf = c::GglBuffer {
            data: key.as_ptr().cast_mut(),
            len: key.len(),
        };
        let mut result: *mut c::GglObject = ptr::null_mut();
        if unsafe { c::ggl_map_get(map, key_buf, &raw mut result) } {
            Some(unsafe {
                NonNull::new(result.cast::<ObjectRef>())
                    .unwrap_unchecked()
                    .as_ref()
            })
        } else {
            None
        }
    }
}

impl<'a> From<&'a [KvRef<'a>]> for MapRef<'a> {
    fn from(slice: &'a [KvRef<'a>]) -> Self {
        MapRef(slice)
    }
}

impl<'a> From<&'a [Kv]> for MapRef<'a> {
    fn from(slice: &'a [Kv]) -> Self {
        unsafe {
            MapRef(slice::from_raw_parts(
                slice.as_ptr().cast::<KvRef>(),
                slice.len(),
            ))
        }
    }
}

impl<'a, const N: usize> From<&'a [Kv; N]> for MapRef<'a> {
    fn from(array: &'a [Kv; N]) -> Self {
        Self::from(&array[..])
    }
}

impl<'a> From<&'a [ObjectRef<'a>]> for ListRef<'a> {
    fn from(slice: &'a [ObjectRef<'a>]) -> Self {
        ListRef(slice)
    }
}

impl<'a> From<&'a [Object]> for ListRef<'a> {
    fn from(slice: &'a [Object]) -> Self {
        unsafe {
            ListRef(slice::from_raw_parts(
                slice.as_ptr().cast::<ObjectRef>(),
                slice.len(),
            ))
        }
    }
}

impl<'a, const N: usize> From<&'a [Object; N]> for ListRef<'a> {
    fn from(array: &'a [Object; N]) -> Self {
        Self::from(&array[..])
    }
}

impl<'a> AsRef<MapRef<'a>> for Map {
    fn as_ref(&self) -> &MapRef<'a> {
        self.borrow()
    }
}

impl<'a> Borrow<MapRef<'a>> for Map {
    fn borrow(&self) -> &MapRef<'a> {
        unsafe {
            (&raw const self.0)
                .cast::<MapRef>()
                .as_ref()
                .unwrap_unchecked()
        }
    }
}

impl<'a> AsRef<ListRef<'a>> for List {
    fn as_ref(&self) -> &ListRef<'a> {
        self.borrow()
    }
}

impl<'a> Borrow<ListRef<'a>> for List {
    fn borrow(&self) -> &ListRef<'a> {
        unsafe {
            (&raw const self.0)
                .cast::<ListRef>()
                .as_ref()
                .unwrap_unchecked()
        }
    }
}

impl Deref for Map {
    type Target = [Kv];
    fn deref(&self) -> &[Kv] {
        &self.0
    }
}

impl<'a> Deref for MapRef<'a> {
    type Target = [KvRef<'a>];
    fn deref(&self) -> &[KvRef<'a>] {
        self.0
    }
}

impl Deref for List {
    type Target = [Object];
    fn deref(&self) -> &[Object] {
        &self.0
    }
}

impl<'a> Deref for ListRef<'a> {
    type Target = [ObjectRef<'a>];
    fn deref(&self) -> &[ObjectRef<'a>] {
        self.0
    }
}
