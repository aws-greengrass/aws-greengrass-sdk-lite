//! Sample component demonstrating how to use RestartComponent

#include <ggl/buffer.h>
#include <ggl/error.h>
#include <ggl/ipc/client.h>
#include <ggl/sdk.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    ggl_sdk_init();

    GglError err = ggipc_connect();
    if (err != GGL_ERR_OK) {
        fprintf(stderr, "Failed to connect to GG nucleus.\n");
        exit(1);
    }
    printf("Connected to GG nucleus.\n");

    printf("Sleeping for 15 seconds before restart...\n");
    for (int i = 15; i > 0; i--) {
        printf("Restart in %d seconds\n", i);
        sleep(1);
    }

    printf("Restarting component "
           "'aws-greengrass-sdk-lite.samples.restart_component'...\n");
    err = ggipc_restart_component(
        GGL_STR("aws-greengrass-sdk-lite.samples.restart_component")
    );
    if (err != GGL_ERR_OK) {
        fprintf(stderr, "Failed to restart component.\n");
        exit(1);
    }

    printf("Restart request sent successfully.\n");
    return 0;
}
