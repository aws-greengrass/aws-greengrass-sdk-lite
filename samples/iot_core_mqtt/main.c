//! Sample component demonstrating pubsub with AWS IoT Core

#include <ggl/arena.h>
#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/client.h>
#include <ggl/sdk.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void response_handler(GglBuffer topic, GglBuffer payload) {
    printf(
        "Received [%.*s] on [%.*s].\n",
        (int) payload.len,
        payload.data,
        (int) topic.len,
        topic.data
    );
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    ggl_sdk_init();

    GglError ret = ggipc_connect();
    if (ret != GGL_ERR_OK) {
        fprintf(stderr, "Failed to connect to GG nucleus.\n");
        exit(1);
    }
    printf("Connected to GG nucleus.\n");

    ret = ggipc_subscribe_to_iot_core(GGL_STR("hello"), 0, &response_handler);
    if (ret != GGL_ERR_OK) {
        fprintf(stderr, "Failed to call subscribe_to_iot_core.\n");
        exit(1);
    }
    printf("Subscribed to topic.\n");

    while (true) {
        static uint8_t publish_mem[5000];
        GglArena publish_arena = ggl_arena_init(GGL_BUF(publish_mem));

        ret = ggipc_publish_to_iot_core(
            GGL_STR("hello"), GGL_STR("world"), 0, publish_arena
        );
        if (ret != GGL_ERR_OK) {
            fprintf(stderr, "Failed to call publish_to_iot_core.\n");
            exit(1);
        }
        printf("Published to topic.\n");

        sleep(15);
    }
}
