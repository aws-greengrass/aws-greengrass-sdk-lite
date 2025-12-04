#include <gg/buffer.hpp>
#include <gg/error.hpp>
#include <gg/ipc/client.hpp>
#include <source_location>
extern "C" {
#include <gg/ipc/mock.h>
#include <gg/ipc/packet_sequences.h>
#include <gg/test.h>
#include <sys/types.h>
#include <unistd.h>
#include <unity.h>

GgError gg_process_wait(pid_t pid) noexcept;
}

namespace {
std::string_view as_view(gg::Buffer buf) noexcept {
    return { reinterpret_cast<char *>(buf.data()), buf.size() };
}
}

extern "C" {
namespace tests {

    namespace {
        GG_TEST_DEFINE(connect_okay) {
            GgipcPacketSequence seq
                = gg_test_connect_accepted_sequence(gg_test_get_auth_token());

            pid_t pid = fork();
            if (pid < 0) {
                TEST_IGNORE_MESSAGE("fork() failed.");
            }

            if (pid == 0) {
                auto &client = gg::ipc::Client::get();
                GG_TEST_ASSERT_OK(client.connect().value());
                TEST_PASS();
            }

            GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(seq, 30));

            GG_TEST_ASSERT_OK(gg_process_wait(pid));
        }

        GG_TEST_DEFINE(connect_with_token_okay) {
            GgipcPacketSequence seq
                = gg_test_connect_accepted_sequence(gg_test_get_auth_token());

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
                    client
                        .connect(
                            as_view(gg_test_get_socket_path()),
                            gg::ipc::AuthToken {
                                as_view(gg_test_get_auth_token()) }
                        )
                        .value()
                );

                TEST_PASS();
            }

            GG_TEST_ASSERT_OK(gg_test_accept_client(1));

            GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(seq, 5));

            GG_TEST_ASSERT_OK(gg_process_wait(pid));
        }

        GG_TEST_DEFINE(connect_bad) {
            GgipcPacketSequence seq = gg_test_connect_hangup_sequence(
                gg::Buffer { gg_test_get_auth_token() }
            );

            pid_t pid = fork();
            if (pid < 0) {
                TEST_IGNORE_MESSAGE("fork() failed.");
            }

            if (pid == 0) {
                auto &client = gg::ipc::Client::get();
                GG_TEST_ASSERT_BAD(client.connect().value());
                TEST_PASS();
            }

            GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(seq, 30));

            /// TODO: verify Classic behavior
            GG_TEST_ASSERT_OK(gg_test_disconnect());

            GG_TEST_ASSERT_OK(gg_process_wait(pid));
        }
    }
}
}
