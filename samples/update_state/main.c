//! Sample component demonstrating how to use UpdateState

#include <gg/error.h>
#include <gg/ipc/client.h>
#include <gg/sdk.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    gg_sdk_init();

    GgError err = ggipc_connect();
    if (err != GG_ERR_OK) {
        fprintf(stderr, "Failed to connect to GG nucleus.\n");
        exit(1);
    }
    printf("Connected to GG nucleus.\n");

    printf("Sleeping for 10 seconds\n");
    sleep(10);

    printf("Updating component state to RUNNING.\n");
    err = ggipc_update_state(GG_COMPONENT_STATE_RUNNING);
    if (err != GG_ERR_OK) {
        fprintf(stderr, "Failed to update component state.\n");
        return EXIT_FAILURE;
    }

    printf("Component state updated successfully\n");

    while (true) {
        sleep(600);
    }
}
