// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

use std::{marker::PhantomData, ops::Deref, ptr::NonNull, str};

use crate::c;

pub trait RefKind<'a>: 'static {
    type Ref<T: ?Sized + 'a>: Deref<Target = T>;

    /// # Safety
    /// The pointer must follow all validity rules
    unsafe fn from_raw<T: ?Sized>(ptr: NonNull<T>) -> Self::Ref<T>;
    fn into_raw<T: ?Sized>(r: Self::Ref<T>) -> NonNull<T>;
}

pub trait MutRefKind<'a>: RefKind<'a> {}

#[derive(Debug)]
pub struct Shared {}
impl<'a> RefKind<'a> for Shared {
    type Ref<T: ?Sized + 'a> = &'a T;

    unsafe fn from_raw<T: ?Sized>(ptr: NonNull<T>) -> &'a T {
        unsafe { ptr.as_ref() }
    }

    fn into_raw<T: ?Sized>(r: &'a T) -> NonNull<T> {
        NonNull::from(r)
    }
}

#[derive(Debug)]
pub struct Mutable {}
impl<'a> RefKind<'a> for Mutable {
    type Ref<T: ?Sized + 'a> = &'a mut T;

    unsafe fn from_raw<T: ?Sized>(mut ptr: NonNull<T>) -> &'a mut T {
        unsafe { ptr.as_mut() }
    }

    fn into_raw<T: ?Sized>(r: &'a mut T) -> NonNull<T> {
        NonNull::from(r)
    }
}
impl MutRefKind<'_> for Mutable {}

#[derive(Debug)]
pub struct Owned {}
impl<'a> RefKind<'a> for Owned {
    type Ref<T: ?Sized + 'a> = Box<T>;

    unsafe fn from_raw<T: ?Sized>(ptr: NonNull<T>) -> Box<T> {
        unsafe { Box::from_raw(ptr.as_ptr()) }
    }

    fn into_raw<T: ?Sized>(r: Box<T>) -> NonNull<T> {
        unsafe { NonNull::new_unchecked(Box::into_raw(r)) }
    }
}
impl MutRefKind<'_> for Owned {}

#[repr(transparent)]
pub struct Object<'a, R: RefKind<'a> = Owned> {
    c: c::GglObject,
    phantom: PhantomData<UnpackedObject<'a, R>>,
}

impl<'a, R: RefKind<'a>> std::fmt::Debug for Object<'a, R> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        self.unpack().fmt(f)
    }
}

#[derive(Debug)]
#[repr(u8)]
pub enum UnpackedObject<'a, R: RefKind<'a> = Owned> {
    Null = 0,
    Bool(bool) = 1,
    I64(i64) = 2,
    F64(f64) = 3,
    Buf(R::Ref<str>) = 4,
    List(R::Ref<[Object<'a, R>]>) = 5,
    Map(R::Ref<[Kv<'a, R>]>) = 6,
}

fn slice_from_c_raw_parts<T>(ptr: *mut T, len: usize) -> NonNull<[T]> {
    let ptr = NonNull::new(ptr).unwrap_or_else(|| {
        assert_eq!(len, 0, "null pointer with non-zero length");
        NonNull::dangling()
    });
    NonNull::slice_from_raw_parts(ptr, len)
}

impl<'a, R: RefKind<'a>> Object<'a, R> {
    pub const NULL: Self = Self {
        c: c::GGL_OBJ_NULL,
        phantom: PhantomData,
    };

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

    #[must_use]
    pub fn buf(buf: impl Into<R::Ref<str>>) -> Self {
        let ptr = R::into_raw(buf.into());
        Self {
            c: unsafe {
                c::ggl_obj_buf(c::GglBuffer {
                    data: ptr.as_ptr().cast::<u8>(),
                    len: ptr.as_ref().len(),
                })
            },
            phantom: PhantomData,
        }
    }

    #[must_use]
    pub fn list(list: impl Into<R::Ref<[Object<'a, R>]>>) -> Self {
        let ptr = R::into_raw(list.into());
        Self {
            c: unsafe {
                c::ggl_obj_list(c::GglList {
                    items: ptr.as_ptr().cast::<c::GglObject>(),
                    len: ptr.len(),
                })
            },
            phantom: PhantomData,
        }
    }

    #[must_use]
    pub fn map(map: impl Into<R::Ref<[Kv<'a, R>]>>) -> Self {
        let ptr = R::into_raw(map.into());
        Self {
            c: unsafe {
                c::ggl_obj_map(c::GglMap {
                    pairs: ptr.as_ptr().cast::<c::GglKV>(),
                    len: ptr.len(),
                })
            },
            phantom: PhantomData,
        }
    }

    pub fn to_owned<'b>(&self) -> Object<'b, Owned> {
        use UnpackedObject::*;
        match self.unpack() {
            Null => Object::NULL,
            Bool(b) => Object::bool(b),
            I64(i) => Object::i64(i),
            F64(f) => Object::f64(f),
            Buf(s) => Object::buf(s.to_owned().into_boxed_str()),
            List(items) => {
                let owned: Box<[Object]> =
                    items.iter().map(Object::to_owned).collect();
                Object::list(owned)
            }
            Map(pairs) => {
                let owned: Box<[Kv]> = pairs.iter().map(Kv::to_owned).collect();
                Object::map(owned)
            }
        }
    }
}

unsafe fn unpack_base<'a, R: RefKind<'a>>(
    obj: c::GglObject,
) -> UnpackedObject<'a, R> {
    use c::GglObjectType::*;
    use UnpackedObject::*;

    unsafe {
        match c::ggl_obj_type(obj) {
            GGL_TYPE_NULL => Null,
            GGL_TYPE_BOOLEAN => Bool(c::ggl_obj_into_bool(obj)),
            GGL_TYPE_I64 => I64(c::ggl_obj_into_i64(obj)),
            GGL_TYPE_F64 => F64(c::ggl_obj_into_f64(obj)),
            GGL_TYPE_BUF => {
                let buf = c::ggl_obj_into_buf(obj);
                let mut ptr = slice_from_c_raw_parts(buf.data, buf.len);
                Buf(R::from_raw(
                    str::from_utf8_mut(ptr.as_mut()).unwrap().into(),
                ))
            }
            GGL_TYPE_LIST => {
                let list = c::ggl_obj_into_list(obj);
                List(R::from_raw(slice_from_c_raw_parts(
                    list.items.cast::<Object<'a, R>>(),
                    list.len,
                )))
            }
            GGL_TYPE_MAP => {
                let map = c::ggl_obj_into_map(obj);
                Map(R::from_raw(slice_from_c_raw_parts(
                    map.pairs.cast::<Kv<'a, R>>(),
                    map.len,
                )))
            }
        }
    }
}

impl<'a, R: RefKind<'a>> Object<'a, R> {
    // Transmuting to Shared is fine since it borrows and no modifications can
    // be made.
    #[must_use]
    pub fn unpack(&self) -> UnpackedObject<'_, Shared> {
        unsafe { unpack_base(self.c) }
    }
}

impl Object<'_, Mutable> {
    // Cannot transmute from Owned to Mutable since a different child could be
    // swapped in which is not from a Box
    #[must_use]
    pub fn unpack_mut(&mut self) -> UnpackedObject<'_, Mutable> {
        unsafe { unpack_base(self.c) }
    }
}

impl<'a> Object<'a, Owned> {
    #[must_use]
    pub fn unpack_owned(self) -> UnpackedObject<'a, Owned> {
        let this = std::mem::ManuallyDrop::new(self);
        unsafe { unpack_base(this.c) }
    }
}

impl Clone for Object<'_, Owned> {
    fn clone(&self) -> Self {
        self.to_owned()
    }
}

impl<'a, R: RefKind<'a>> Drop for Object<'a, R> {
    fn drop(&mut self) {
        let _: UnpackedObject<'a, R> = unsafe { unpack_base(self.c) };
    }
}

impl<'a, R: RefKind<'a>> Default for Object<'a, R> {
    fn default() -> Self {
        Self::NULL
    }
}

#[repr(transparent)]
pub struct Kv<'a, R: RefKind<'a> = Owned> {
    c: c::GglKV,
    phantom_key: PhantomData<R::Ref<str>>,
    phantom_value: PhantomData<Object<'a, R>>,
}

impl<'a, R: RefKind<'a>> std::fmt::Debug for Kv<'a, R> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Kv")
            .field("key", &self.key())
            .field("val", self.val())
            .finish()
    }
}

impl<'a, R: RefKind<'a>> Kv<'a, R> {
    #[allow(clippy::needless_pass_by_value)]
    pub fn new(key: impl Into<R::Ref<str>>, value: Object<'a, R>) -> Self {
        let s = &*key.into();
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
            str::from_utf8(std::slice::from_raw_parts(buf.data, buf.len))
                .unwrap()
        }
    }

    pub fn set_key(&mut self, key: impl Into<R::Ref<str>>) {
        let s = &*key.into();
        unsafe {
            c::ggl_kv_set_key(
                &raw mut self.c,
                c::GglBuffer {
                    data: s.as_ptr().cast_mut(),
                    len: s.len(),
                },
            );
        }
    }

    #[must_use]
    pub fn val(&self) -> &Object<'a, R> {
        unsafe {
            &*(c::ggl_kv_val((&raw const self.c).cast_mut())
                as *const Object<'a, R>)
        }
    }

    pub fn val_mut(&mut self) -> &mut Object<'a, R> {
        unsafe { &mut *c::ggl_kv_val(&raw mut self.c).cast::<Object<'a, R>>() }
    }

    #[must_use]
    pub fn to_owned<'b>(&self) -> Kv<'b, Owned> {
        Kv::new(
            self.key().to_owned().into_boxed_str(),
            self.val().to_owned(),
        )
    }
}

impl Clone for Kv<'_, Owned> {
    fn clone(&self) -> Self {
        self.to_owned()
    }
}

impl<'a, R: MutRefKind<'a>> Kv<'a, R> {
    /// # Panics
    /// Panics if the key is not valid UTF-8.
    pub fn key_mut(&mut self) -> &mut str {
        let buf = unsafe { c::ggl_kv_key(self.c) };
        unsafe {
            str::from_utf8_mut(std::slice::from_raw_parts_mut(
                buf.data, buf.len,
            ))
            .unwrap()
        }
    }
}

pub trait MapExt<'a, R: RefKind<'a>> {
    fn map_get(&self, key: &str) -> Option<&Object<'a, R>>;
    fn map_get_mut(&mut self, key: &str) -> Option<&mut Object<'a, R>>;
}

#[allow(clippy::needless_pass_by_value)]
fn map_get_ref<'a, 'b: 'a, ArgRef: RefKind<'a>, ObjRef: RefKind<'b>>(
    map: ArgRef::Ref<[Kv<'b, ObjRef>]>,
    key: &str,
) -> Option<ArgRef::Ref<Object<'b, ObjRef>>> {
    let map = c::GglMap {
        pairs: map.as_ptr() as *mut c::GglKV,
        len: map.len(),
    };
    let key = c::GglBuffer {
        data: key.as_ptr().cast_mut(),
        len: key.len(),
    };
    let mut result: *mut c::GglObject = std::ptr::null_mut();
    if unsafe { c::ggl_map_get(map, key, &raw mut result) } {
        Some(unsafe {
            ArgRef::from_raw(
                NonNull::new(result.cast::<Object<'b, ObjRef>>()).unwrap(),
            )
        })
    } else {
        None
    }
}

impl<'a, R: RefKind<'a>> MapExt<'a, R> for [Kv<'a, R>] {
    fn map_get(&self, key: &str) -> Option<&Object<'a, R>> {
        map_get_ref::<Shared, _>(self, key)
    }

    fn map_get_mut(&mut self, key: &str) -> Option<&mut Object<'a, R>> {
        map_get_ref::<Mutable, _>(self, key)
    }
}
