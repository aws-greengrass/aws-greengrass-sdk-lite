// aws-greengrass-sdk-lite - Lightweight AWS IoT Greengrass SDK
// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef GG_VECTOR_H
#define GG_VECTOR_H

//! Generic Object Vector interface

#include <gg/attr.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/io.h>
#include <gg/object.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __COVERITY__
#define GG_DISABLE_MACRO_TYPE_CHECKING
#endif

typedef struct {
    GgList list;
    size_t capacity;
} GgObjVec;

#define GG_OBJ_VEC_UNCHECKED(...) \
    ((GgObjVec) { .list = { .items = (__VA_ARGS__), .len = 0 }, \
                  .capacity = sizeof(__VA_ARGS__) / sizeof(GgObject) })

#ifndef GG_DISABLE_MACRO_TYPE_CHECKING
#define GG_OBJ_VEC(...) \
    _Generic((&(__VA_ARGS__)), GgObject(*)[]: GG_OBJ_VEC_UNCHECKED(__VA_ARGS__))
#else
#define GG_OBJ_VEC GG_OBJ_VEC_UNCHECKED
#endif

VISIBILITY(hidden)
GgError gg_obj_vec_push(GgObjVec *vector, GgObject object);
VISIBILITY(hidden)
void gg_obj_vec_chain_push(GgError *err, GgObjVec *vector, GgObject object);
VISIBILITY(hidden)
GgError gg_obj_vec_pop(GgObjVec *vector, GgObject *out);
VISIBILITY(hidden)
GgError gg_obj_vec_append(GgObjVec *vector, GgList list);
VISIBILITY(hidden)
void gg_obj_vec_chain_append(GgError *err, GgObjVec *vector, GgList list);

typedef struct {
    GgMap map;
    size_t capacity;
} GgKVVec;

#define GG_KV_VEC_UNCHECKED(...) \
    ((GgKVVec) { .map = { .pairs = (__VA_ARGS__), .len = 0 }, \
                 .capacity = sizeof(__VA_ARGS__) / sizeof(GgKV) })

#ifndef GG_DISABLE_MACRO_TYPE_CHECKING
#define GG_KV_VEC(...) \
    _Generic((&(__VA_ARGS__)), GgKV(*)[]: GG_KV_VEC_UNCHECKED(__VA_ARGS__))
#else
#define GG_KV_VEC GG_KV_VEC_UNCHECKED
#endif

VISIBILITY(hidden)
GgError gg_kv_vec_push(GgKVVec *vector, GgKV kv);

typedef struct {
    GgBuffer buf;
    size_t capacity;
} GgByteVec;

#define GG_BYTE_VEC_UNCHECKED(...) \
    ((GgByteVec) { .buf = { .data = (uint8_t *) (__VA_ARGS__), .len = 0 }, \
                   .capacity = sizeof(__VA_ARGS__) })

#ifndef GG_DISABLE_MACRO_TYPE_CHECKING
#define GG_BYTE_VEC(...) \
    _Generic( \
        (&(__VA_ARGS__)), \
        uint8_t (*)[]: GG_BYTE_VEC_UNCHECKED(__VA_ARGS__), \
        char (*)[]: GG_BYTE_VEC_UNCHECKED(__VA_ARGS__) \
    )
#else
#define GG_BYTE_VEC GG_BYTE_VEC_UNCHECKED
#endif

VISIBILITY(hidden)
GgByteVec gg_byte_vec_init(GgBuffer buf);
VISIBILITY(hidden)
GgError gg_byte_vec_push(GgByteVec *vector, uint8_t byte);
VISIBILITY(hidden)
void gg_byte_vec_chain_push(GgError *err, GgByteVec *vector, uint8_t byte);
VISIBILITY(hidden)
GgError gg_byte_vec_append(GgByteVec *vector, GgBuffer buf);
VISIBILITY(hidden)
void gg_byte_vec_chain_append(GgError *err, GgByteVec *vector, GgBuffer buf);
VISIBILITY(hidden)
GgBuffer gg_byte_vec_remaining_capacity(GgByteVec vector);

/// Returns a writer that writes into a GgByteVec
VISIBILITY(hidden)
GgWriter gg_byte_vec_writer(GgByteVec *byte_vec);

typedef struct {
    GgBufList buf_list;
    size_t capacity;
} GgBufVec;

#define GG_BUF_VEC_UNCHECKED(...) \
    ((GgBufVec) { .buf_list = { .bufs = (__VA_ARGS__), .len = 0 }, \
                  .capacity = sizeof(__VA_ARGS__) / sizeof(GgBuffer) })

#ifndef GG_DISABLE_MACRO_TYPE_CHECKING
#define GG_BUF_VEC(...) \
    _Generic((&(__VA_ARGS__)), GgBuffer(*)[]: GG_BUF_VEC_UNCHECKED(__VA_ARGS__))
#else
#define GG_BUF_VEC GG_BUF_VEC_UNCHECKED
#endif

VISIBILITY(hidden)
GgError gg_buf_vec_push(GgBufVec *vector, GgBuffer buf);
VISIBILITY(hidden)
void gg_buf_vec_chain_push(GgError *err, GgBufVec *vector, GgBuffer buf);
VISIBILITY(hidden)
GgError gg_buf_vec_append_list(GgBufVec *vector, GgList list);
VISIBILITY(hidden)
void gg_buf_vec_chain_append_list(GgError *err, GgBufVec *vector, GgList list);

#endif
