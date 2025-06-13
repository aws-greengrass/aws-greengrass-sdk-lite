//! Sample component demonstrating how to use UpdateState

#include <ggl/error.h>
#include <ggl/ipc/client.h>
#include <ggl/sdk.h>
#include <unistd.h>
#include <stdbool.h>
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

    printf("Sleeping for 10 seconds\n");
    sleep(10);

    printf("Updating component state to RUNNING.\n");
    err = ggipc_update_state(GGL_COMPONENT_STATE_RUNNING);
    if (err != GGL_ERR_OK) {
        fprintf(stderr, "Failed to update component state.\n");
        return EXIT_FAILURE;
    }

    printf("Component state updated successfully\n");

    while (true) {
        sleep(600);
    }
}
