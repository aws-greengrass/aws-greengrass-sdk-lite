//! Sample component demonstrating reading/writing component config

#include <ggl/arena.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/client.h>
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

    uint8_t str_resp_mem[500];
    GglBuffer test_str = GGL_BUF(str_resp_mem);
    ret = ggipc_get_config_str(
        GGL_BUF_LIST(GGL_STR("test_str")), NULL, &test_str
    );
    if (ret != GGL_ERR_OK) {
        fprintf(stderr, "Failed to call get_config for test_str.\n");
        exit(1);
    }
    printf("test_str value is %.*s.\n", (int) test_str.len, test_str.data);

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
        static uint8_t resp_mem[500];
        GglArena resp_arena = ggl_arena_init(GGL_BUF(resp_mem));
        GglObject resp;
        ret = ggipc_get_config(
            GGL_BUF_LIST(GGL_STR("test_num")), NULL, &resp_arena, &resp
        );
        if (ret != GGL_ERR_OK) {
            fprintf(stderr, "Failed to call get_config for test_num.\n");
            exit(1);
        }
        if (ggl_obj_type(resp) != GGL_TYPE_I64) {
            fprintf(stderr, "Config value test_num is unexpected type.\n");
            exit(1);
        }
        int64_t val = ggl_obj_into_i64(resp);
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
