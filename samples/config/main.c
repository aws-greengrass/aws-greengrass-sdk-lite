//! Sample component demonstrating reading/writing component config

#include <gg/arena.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/ipc/client.h>
#include <gg/map.h>
#include <gg/object.h>
#include <gg/sdk.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    gg_sdk_init();

    GgError ret = ggipc_connect();
    if (ret != GG_ERR_OK) {
        fprintf(stderr, "Failed to connect to GG nucleus.\n");
        exit(1);
    }
    printf("Connected to GG nucleus.\n");

    uint8_t response_mem[500];
    GgBuffer test_str = GG_BUF(response_mem);
    ret = ggipc_get_config_str(
        GG_BUF_LIST(GG_STR("test_str")), NULL, &test_str
    );
    if (ret != GG_ERR_OK) {
        fprintf(stderr, "Failed to call get_config for test_str.\n");
        exit(1);
    }
    printf("test_str value is %.*s.\n", (int) test_str.len, test_str.data);

    GgBuffer subkey2_str = GG_BUF(response_mem);
    ret = ggipc_get_config_str(
        GG_BUF_LIST(
            GG_STR("sample_map"), GG_STR("key2_map"), GG_STR("subkey2")
        ),
        NULL,
        &subkey2_str
    );
    if (ret == GG_ERR_OK) {
        printf(
            "subkey2 value is %.*s.\n", (int) subkey2_str.len, subkey2_str.data
        );
    }

    GgArena key2_map_arena = gg_arena_init(GG_BUF(response_mem));
    GgObject key2_map_obj;
    ret = ggipc_get_config(
        GG_BUF_LIST(GG_STR("sample_map"), GG_STR("key2_map")),
        NULL,
        &key2_map_arena,
        &key2_map_obj
    );
    if (ret == GG_ERR_OK && gg_obj_type(key2_map_obj) == GG_TYPE_MAP) {
        GgMap key2_map = gg_obj_into_map(key2_map_obj);
        printf("key2_map has %zu entries\n", key2_map.len);
        for (size_t i = 0; i < key2_map.len; i++) {
            GgBuffer key = gg_kv_key(key2_map.pairs[i]);
            GgObject *value = gg_kv_val(&key2_map.pairs[i]);
            if (gg_obj_type(*value) == GG_TYPE_BUF) {
                GgBuffer val_buf = gg_obj_into_buf(*value);
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

    GgArena key3_arena = gg_arena_init(GG_BUF(response_mem));
    GgObject key3_obj;
    ret = ggipc_get_config(
        GG_BUF_LIST(GG_STR("sample_map"), GG_STR("key3")),
        NULL,
        &key3_arena,
        &key3_obj
    );
    if (ret == GG_ERR_OK && gg_obj_type(key3_obj) == GG_TYPE_I64) {
        int64_t key3_val = gg_obj_into_i64(key3_obj);
        printf("key3 value is %" PRId64 ".\n", key3_val);
    }

    int64_t init_val = 1;
    printf("Initializing test_num value to %" PRId64 ".\n", init_val);

    ret = ggipc_update_config(
        GG_BUF_LIST(GG_STR("test_num")), NULL, gg_obj_i64(init_val)
    );
    if (ret != GG_ERR_OK) {
        fprintf(stderr, "Failed to call update_config for test_num.\n");
        exit(1);
    }

    while (true) {
        GgArena resp_arena = gg_arena_init(GG_BUF(response_mem));
        GgObject resp;

        ret = ggipc_get_config(
            GG_BUF_LIST(GG_STR("test_num")), NULL, &resp_arena, &resp
        );
        if (ret != GG_ERR_OK) {
            fprintf(stderr, "Failed to call get_config for test_num.\n");
            exit(1);
        }

        int64_t val;
        if (gg_obj_type(resp) == GG_TYPE_I64) {
            val = gg_obj_into_i64(resp);
        } else if (gg_obj_type(resp) == GG_TYPE_F64) {
            val = (int64_t) gg_obj_into_f64(resp);
        } else {
            fprintf(
                stderr,
                "Config value test_num is unexpected type: %d\n",
                gg_obj_type(resp)
            );
            exit(1);
        }
        printf("test_num value is %" PRId64 ".\n", val);

        val += 1;

        printf("Setting test_num value to %" PRId64 ".\n", val);

        ret = ggipc_update_config(
            GG_BUF_LIST(GG_STR("test_num")), NULL, gg_obj_i64(val)
        );
        if (ret != GG_ERR_OK) {
            fprintf(stderr, "Failed to call update_config for test_num.\n");
            exit(1);
        }

        sleep(15);
    }
}
