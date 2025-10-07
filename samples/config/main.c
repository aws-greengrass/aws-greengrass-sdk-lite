//! Sample component demonstrating reading/writing component config

#include <ggl/arena.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/client.h>
#include <ggl/map.h>
#include <ggl/object.h>
#include <ggl/sdk.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    ggl_sdk_init();

    GglError ret = ggipc_connect();
    if (ret != GGL_ERR_OK) {
        fprintf(stderr, "Failed to connect to GG nucleus.\n");
        exit(1);
    }
    printf("Connected to GG nucleus.\n");

    uint8_t response_mem[500];
    GglBuffer test_str = GGL_BUF(response_mem);
    ret = ggipc_get_config_str(
        GGL_BUF_LIST(GGL_STR("test_str")), NULL, &test_str
    );
    if (ret != GGL_ERR_OK) {
        fprintf(stderr, "Failed to call get_config for test_str.\n");
        exit(1);
    }
    printf("test_str value is %.*s.\n", (int) test_str.len, test_str.data);

    GglBuffer subkey2_str = GGL_BUF(response_mem);
    ret = ggipc_get_config_str(
        GGL_BUF_LIST(
            GGL_STR("sample_map"), GGL_STR("key2_map"), GGL_STR("subkey2")
        ),
        NULL,
        &subkey2_str
    );
    if (ret == GGL_ERR_OK) {
        printf(
            "subkey2 value is %.*s.\n", (int) subkey2_str.len, subkey2_str.data
        );
    }

    GglArena key2_map_arena = ggl_arena_init(GGL_BUF(response_mem));
    GglObject key2_map_obj;
    ret = ggipc_get_config(
        GGL_BUF_LIST(GGL_STR("sample_map"), GGL_STR("key2_map")),
        NULL,
        &key2_map_arena,
        &key2_map_obj
    );
    if (ret == GGL_ERR_OK && ggl_obj_type(key2_map_obj) == GGL_TYPE_MAP) {
        GglMap key2_map = ggl_obj_into_map(key2_map_obj);
        printf("key2_map has %zu entries\n", key2_map.len);
        for (size_t i = 0; i < key2_map.len; i++) {
            GglBuffer key = ggl_kv_key(key2_map.pairs[i]);
            GglObject *value = ggl_kv_val(&key2_map.pairs[i]);
            if (ggl_obj_type(*value) == GGL_TYPE_BUF) {
                GglBuffer val_buf = ggl_obj_into_buf(*value);
                printf(
                    "  %.*s: %.*s\n",
                    (int) key.len,
                    key.data,
                    (int) val_buf.len,
                    val_buf.data
                );
            }
        }
    }

    GglArena key3_arena = ggl_arena_init(GGL_BUF(response_mem));
    GglObject key3_obj;
    ret = ggipc_get_config(
        GGL_BUF_LIST(GGL_STR("sample_map"), GGL_STR("key3")),
        NULL,
        &key3_arena,
        &key3_obj
    );
    if (ret == GGL_ERR_OK && ggl_obj_type(key3_obj) == GGL_TYPE_I64) {
        int64_t key3_val = ggl_obj_into_i64(key3_obj);
        printf("key3 value is %" PRId64 ".\n", key3_val);
    }

    int64_t init_val = 1;
    printf("Initializing test_num value to %" PRId64 ".\n", init_val);

    ret = ggipc_update_config(
        GGL_BUF_LIST(GGL_STR("test_num")), NULL, ggl_obj_i64(init_val)
    );
    if (ret != GGL_ERR_OK) {
        fprintf(stderr, "Failed to call update_config for test_num.\n");
        exit(1);
    }

    while (true) {
        GglArena resp_arena = ggl_arena_init(GGL_BUF(response_mem));
        GglObject resp;

        ret = ggipc_get_config(
            GGL_BUF_LIST(GGL_STR("test_num")), NULL, &resp_arena, &resp
        );
        if (ret != GGL_ERR_OK) {
            fprintf(stderr, "Failed to call get_config for test_num.\n");
            exit(1);
        }

        int64_t val;
        if (ggl_obj_type(resp) == GGL_TYPE_I64) {
            val = ggl_obj_into_i64(resp);
        } else if (ggl_obj_type(resp) == GGL_TYPE_F64) {
            val = (int64_t) ggl_obj_into_f64(resp);
        } else {
            fprintf(
                stderr,
                "Config value test_num is unexpected type: %d\n",
                ggl_obj_type(resp)
            );
            exit(1);
        }
        printf("test_num value is %" PRId64 ".\n", val);

        val += 1;

        printf("Setting test_num value to %" PRId64 ".\n", val);

        ret = ggipc_update_config(
            GGL_BUF_LIST(GGL_STR("test_num")), NULL, ggl_obj_i64(val)
        );
        if (ret != GGL_ERR_OK) {
            fprintf(stderr, "Failed to call update_config for test_num.\n");
            exit(1);
        }

        sleep(15);
    }
}
