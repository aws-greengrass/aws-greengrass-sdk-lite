#ifndef GGL_TYPES_HPP
#define GGL_TYPES_HPP

#include "ggl/error.hpp"
#include <cstddef>
#include <cstdint>

extern "C" {
typedef struct {
    uint8_t _private[(sizeof(void *) == 4) ? 9 : 11];
} GglObject;

typedef struct {
    uint8_t *data;
    size_t len;
} GglBuffer;

typedef struct {
    GglObject *items;
    size_t len;
} GglList;

typedef struct {
    uint8_t _private[sizeof(void *) + 2 + sizeof(GglObject)];
} GglKV;

typedef struct {
    GglKV *pairs;
    size_t len;
} GglMap;

typedef struct {
    void *mem;
    uint32_t capacity;
    uint32_t index;
} GglArena;

/// Type tag for `GglObject`.
typedef enum {
    GGL_TYPE_NULL = 0,
    GGL_TYPE_BOOLEAN,
    GGL_TYPE_I64,
    GGL_TYPE_F64,
    GGL_TYPE_BUF,
    GGL_TYPE_LIST,
    GGL_TYPE_MAP,
} GglObjectType;

GglObjectType ggl_obj_type(GglObject);
bool ggl_obj_into_bool(GglObject);
int64_t ggl_obj_into_i64(GglObject);
double ggl_obj_into_f64(GglObject);
GglBuffer ggl_obj_into_buf(GglObject);
GglList ggl_obj_into_list(GglObject);
GglMap ggl_obj_into_map(GglObject);

GglObject ggl_obj_bool(bool);
GglObject ggl_obj_i64(int64_t);
GglObject ggl_obj_f64(double);
GglObject ggl_obj_buf(GglBuffer);
GglObject ggl_obj_list(GglList);
GglObject ggl_obj_map(GglMap);

GglKV ggl_kv(GglBuffer, GglObject);
GglObject *ggl_kv_val(GglKV *);
GglBuffer ggl_kv_key(GglKV);
void ggl_kv_set_key(GglKV *kv, GglBuffer key);

GglError ggl_list_type_check(GglList list, GglObjectType type);

void *ggl_arena_alloc(GglArena *arena, size_t size, size_t alignment);
}

#endif
