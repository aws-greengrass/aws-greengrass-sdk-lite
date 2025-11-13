//! Sample component demonstrating pubsub with AWS IoT Core

#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/ipc/client.h>
#include <gg/sdk.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void response_handler(
    void *ctx, GgBuffer topic, GgBuffer payload, GgIpcSubscriptionHandle handle
) {
    (void) ctx;
    (void) handle;
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

    gg_sdk_init();

    GgError ret = ggipc_connect();
    if (ret != GG_ERR_OK) {
        fprintf(stderr, "Failed to connect to GG nucleus.\n");
        exit(1);
    }
    printf("Connected to GG nucleus.\n");

    ret = ggipc_subscribe_to_iot_core(
        GG_STR("hello"), 0, &response_handler, NULL, NULL
    );
    if (ret != GG_ERR_OK) {
        fprintf(stderr, "Failed to call subscribe_to_iot_core.\n");
        exit(1);
    }
    printf("Subscribed to topic.\n");

    while (true) {
        ret = ggipc_publish_to_iot_core(GG_STR("hello"), GG_STR("world"), 0);
        if (ret != GG_ERR_OK) {
            fprintf(stderr, "Failed to call publish_to_iot_core.\n");
            exit(1);
        }
        printf("Published to topic.\n");

        sleep(15);
    }
}
