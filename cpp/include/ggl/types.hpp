#ifndef GGL_TYPES_HPP
#define GGL_TYPES_HPP

#include <ggl/error.hpp>
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
    GglBuffer *bufs;
    size_t len;
} GglBufList;

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

enum class GglComponentState {
    RUNNING,
    ERRORED
};

namespace ggl {
    using ComponentState = GglComponentState;
}

[[gnu::const]]
GglObjectType ggl_obj_type(GglObject) noexcept;
[[gnu::const]]
bool ggl_obj_into_bool(GglObject) noexcept;
[[gnu::const]]
int64_t ggl_obj_into_i64(GglObject) noexcept;
[[gnu::const]]
double ggl_obj_into_f64(GglObject) noexcept;
[[gnu::const]]
GglBuffer ggl_obj_into_buf(GglObject) noexcept;
[[gnu::const]]
GglList ggl_obj_into_list(GglObject) noexcept;
[[gnu::const]]
GglMap ggl_obj_into_map(GglObject) noexcept;

[[gnu::const]]
GglObject ggl_obj_bool(bool) noexcept;
[[gnu::const]]
GglObject ggl_obj_i64(int64_t) noexcept;
[[gnu::const]]
GglObject ggl_obj_f64(double) noexcept;
[[gnu::const]]
GglObject ggl_obj_buf(GglBuffer) noexcept;
[[gnu::const]]
GglObject ggl_obj_list(GglList) noexcept;
[[gnu::const]]
GglObject ggl_obj_map(GglMap) noexcept;

[[gnu::const]]
GglKV ggl_kv(GglBuffer, GglObject) noexcept;
[[gnu::const]]
GglObject *ggl_kv_val(GglKV *) noexcept;
[[gnu::const]]
GglBuffer ggl_kv_key(GglKV) noexcept;
void ggl_kv_set_key(GglKV *kv, GglBuffer key) noexcept;

[[gnu::pure]]
GglError ggl_list_type_check(GglList list, GglObjectType type) noexcept;

void *ggl_arena_alloc(GglArena *arena, size_t size, size_t alignment) noexcept;
}

#endif
