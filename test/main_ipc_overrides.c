#include <gg/ipc/mock.h>
#include <gg/test.h>
#include <unistd.h>
#include <stdlib.h>

#ifndef GG_TEST_SOCKET_DIR
#define GG_TEST_SOCKET_DIR "/tmp/gg-test"
#endif

#ifndef GG_TEST_AUTH_TOKEN
#define GG_TEST_AUTH_TOKEN "1234567890ABCDEF"
#endif

void suiteSetUp(void) {
    GgError ret
        = gg_test_setup_ipc(GG_TEST_SOCKET_DIR, 0777, GG_TEST_AUTH_TOKEN);
    if (ret != GG_ERR_OK) {
        _Exit(1);
    }
}

void tearDown(void) {
    (void) gg_test_disconnect();
}

int suiteTearDown(int num_failures) {
    gg_test_close();
    return num_failures;
}

int main(void) {
    return gg_test_run_suite();
}
