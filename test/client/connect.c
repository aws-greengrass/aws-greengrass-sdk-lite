#include <errno.h>
#include <gg/arena.h>
#include <gg/buffer.h>
#include <gg/error.h>
#include <gg/eventstream/rpc.h>
#include <gg/file.h>
#include <gg/ipc/client.h>
#include <gg/ipc/mock.h>
#include <gg/ipc/packet_sequences.h>
#include <gg/log.h>
#include <gg/map.h>
#include <gg/object.h>
#include <gg/process_wait.h>
#include <gg/sdk.h>
#include <gg/test.h>
#include <pthread.h>
#include <unistd.h>
#include <unity.h>
#include <stdlib.h>

GG_TEST_DEFINE(connect_okay) {
    GgipcPacketSequence seq
        = gg_test_connect_accepted_sequence(gg_test_get_auth_token());

    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        gg_sdk_init();
        GG_TEST_ASSERT_OK(ggipc_connect());
        TEST_PASS();
    }

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(seq, 5, server_handle));

    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}

GG_TEST_DEFINE(connect_with_token_okay) {
    GgipcPacketSequence seq
        = gg_test_connect_accepted_sequence(gg_test_get_auth_token());

    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        // make it impossible for client to get these env variables
        // done before SDK can make any threads
        // NOLINTBEGIN(concurrency-mt-unsafe)
        unsetenv("SVCUID");
        unsetenv("AWS_GG_NUCLEUS_DOMAIN_SOCKET_FILEPATH_FOR_COMPONENT");
        // NOLINTEND(concurrency-mt-unsafe)
        gg_sdk_init();
        GG_TEST_ASSERT_OK(ggipc_connect_with_token(
            gg_test_get_socket_path(), gg_test_get_auth_token()
        ));
        TEST_PASS();
    }

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(seq, 5, server_handle));

    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}

GG_TEST_DEFINE(connect_bad) {
    GgipcPacketSequence seq
        = gg_test_connect_hangup_sequence(gg_test_get_auth_token());

    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        gg_sdk_init();
        GG_TEST_ASSERT_BAD(ggipc_connect());
        TEST_PASS();
    }

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(seq, 1, server_handle));

    /// TODO: verify Classic behavior
    GG_TEST_ASSERT_OK(gg_test_disconnect(server_handle));

    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}

GG_TEST_DEFINE(connect_with_token_bad) {
    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        // NOLINTBEGIN(concurrency-mt-unsafe)
        unsetenv("SVCUID");
        unsetenv("AWS_GG_NUCLEUS_DOMAIN_SOCKET_FILEPATH_FOR_COMPONENT");
        // NOLINTEND(concurrency-mt-unsafe)

        gg_sdk_init();
        GG_TEST_ASSERT_BAD(ggipc_connect());
        TEST_PASS();
    }

    GG_TEST_ASSERT_BAD(gg_test_accept_client(1, server_handle));

    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}
