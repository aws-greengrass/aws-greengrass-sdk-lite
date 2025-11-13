//! Sample component demonstrating how to use RestartComponent

#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/ipc/client.h>
#include <gg/sdk.h>
#include <unistd.h>
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

    printf("Sleeping for 15 seconds before restart...\n");
    for (int i = 15; i > 0; i--) {
        printf("Restart in %d seconds\n", i);
        sleep(1);
    }

    printf(
        "Restarting component 'aws-greengrass-sdk-lite.samples.restart_component'...\n"
    );
    err = ggipc_restart_component(
        GG_STR("aws-greengrass-sdk-lite.samples.restart_component")
    );
    if (err != GG_ERR_OK) {
        fprintf(stderr, "Failed to restart component.\n");
        exit(1);
    }

    printf("Restart request sent successfully.\n");
    return 0;
}
