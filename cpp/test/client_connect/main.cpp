#include <gg/error.hpp>
#include <gg/ipc/client.hpp>
#include <source_location>
extern "C" {
#include <gg/ipc/mock.h>
#include <gg/ipc/packet_sequences.h>
#include <gg/process_wait.h>
#include <unistd.h>
#include <unity.h>
}

namespace {
int server_handle = -1;

constexpr const char *auth_token = "0123456789ABCDEF";
constexpr const char *socket_path = "cpp_connect_socket.ipc";
}

#define GG_TEST_ASSERT_OK(expr) TEST_ASSERT_EQUAL(GG_ERR_OK, (expr))
#define GG_TEST_ASSERT_BAD(expr) TEST_ASSERT_NOT_EQUAL(GG_ERR_OK, (expr))

void suiteSetUp(void) {
    std::ios_base::sync_with_stdio(false);
    GgError ret
        = gg_test_setup_ipc(socket_path, 0777, &server_handle, auth_token);
    if ((ret != GG_ERR_OK) || (server_handle < 0)) {
        _Exit(1);
    }
}

void setUp(void) {
}

void tearDown(void) {
}

int suiteTearDown(int num_failures) {
    gg_test_close(server_handle);
    server_handle = -1;
    return num_failures;
}

namespace tests {

namespace {
    void connect_okay(void) {
        GgipcPacketSequence seq
            = gg_test_conn_ack_sequence(gg::Buffer { auth_token });

        pid_t pid = fork();
        if (pid < 0) {
            TEST_IGNORE_MESSAGE("fork() failed.");
        }
        if (pid == 0) {
            auto &client = gg::ipc::Client::get();
            GG_TEST_ASSERT_OK(client.connect().value());
            TEST_PASS();
        }

        GG_TEST_ASSERT_OK(
            gg_test_expect_packet_sequence(seq, 30, server_handle)
        );

        bool clean_exit = false;
        GG_TEST_ASSERT_OK(gg_process_wait(pid, &clean_exit));
        TEST_ASSERT_TRUE(clean_exit);
    }

    void connect_with_token_okay(void) {
        GgipcPacketSequence seq
            = gg_test_conn_ack_sequence(gg::Buffer { auth_token });

        pid_t pid = fork();
        if (pid < 0) {
            TEST_IGNORE_MESSAGE("fork() failed.");
        }

        if (pid == 0) {
            // make it impossible for client to get these env variables
            // done before SDK can make any threads
            // NOLINTBEGIN(concurrency-mt-unsafe)
            unsetenv("SVCUID");
            unsetenv("AWS_GG_NUCLEUS_DOMAIN_SOCKET_FILEPATH_FOR_COMPONENT");
            // NOLINTEND(concurrency-mt-unsafe)
            auto &client = gg::ipc::Client::get();
            GG_TEST_ASSERT_OK(
                client.connect(socket_path, gg::ipc::AuthToken { auth_token })
                    .value()
            );

            TEST_PASS();
        }

        GG_TEST_ASSERT_OK(
            gg_test_expect_packet_sequence(seq, 5, server_handle)
        );

        bool clean_exit = false;
        GG_TEST_ASSERT_OK(gg_process_wait(pid, &clean_exit));
        TEST_ASSERT_TRUE(clean_exit);
    }

    void connect_bad(void) {
        GgipcPacketSequence seq
            = gg_test_connect_hangup_sequence(gg::Buffer { auth_token });

        pid_t pid = fork();
        if (pid < 0) {
            TEST_IGNORE_MESSAGE("fork() failed.");
        }

        if (pid == 0) {
            auto &client = gg::ipc::Client::get();
            GG_TEST_ASSERT_BAD(client.connect().value());
            TEST_PASS();
        }

        GG_TEST_ASSERT_OK(
            gg_test_expect_packet_sequence(seq, 30, server_handle)
        );

        /// TODO: verify Classic behavior
        GG_TEST_ASSERT_OK(gg_test_disconnect(server_handle));

        bool clean_exit = false;
        GG_TEST_ASSERT_OK(gg_process_wait(pid, &clean_exit));
        TEST_ASSERT_TRUE(clean_exit);
    }
}
}

int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    suiteSetUp();

    UNITY_BEGIN();
    RUN_TEST(tests::connect_okay);
    RUN_TEST(tests::connect_with_token_okay);
    RUN_TEST(tests::connect_bad);
    int num_failures = UNITY_END();

    return suiteTearDown(num_failures);
}
