//! Sample component demonstrating pubsub with AWS IoT Core

#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/client.h>
#include <ggl/sdk.h>
#include <unistd.h>
#include <stdbool.h>
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
        ret = ggipc_publish_to_iot_core(GGL_STR("hello"), GGL_STR("world"), 0);
        if (ret != GGL_ERR_OK) {
            fprintf(stderr, "Failed to call publish_to_iot_core.\n");
            exit(1);
        }
        printf("Published to topic.\n");

        sleep(15);
    }
}
