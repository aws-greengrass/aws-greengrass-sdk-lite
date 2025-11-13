#ifndef GG_OBJECT_ITER_H
#define GG_OBJECT_ITER_H

#include <gg/object.h>
#include <stdint.h>

typedef enum {
    LEVEL_DEFAULT,
    LEVEL_LIST,
    LEVEL_MAP,
} IterLevelState;

typedef struct {
    GgObject *obj[GG_MAX_OBJECT_DEPTH];
    uint8_t state[GG_MAX_OBJECT_DEPTH];
    uint8_t elem_index[GG_MAX_OBJECT_DEPTH];
    uint8_t index;
} IterLevels;

#endif
