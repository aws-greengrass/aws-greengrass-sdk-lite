#include <gg/buffer.hpp>
#include <gg/error.hpp>
#include <gg/ipc/client.hpp>
#include <gg/map.hpp>
#include <gg/object.hpp>
#include <gg/types.hpp>
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
template <class T> void test_config_overload(T expected) {
    std::array keys = {
        gg::Buffer { "key" },
    };

    T value {};

    pid_t pid = fork();
    if (pid < 0) {
        TEST_IGNORE_MESSAGE("fork() failed.");
    }

    if (pid == 0) {
        auto &client = gg::ipc::Client::get();
        GG_TEST_ASSERT_OK(client.connect().value());

        try {
            GG_TEST_ASSERT_OK(
                client.get_config(keys, std::nullopt, value).value()
            );
        } catch (...) {
            TEST_FAIL_MESSAGE("Exception caught from get_config");
        }

        if constexpr (std::is_same_v<T, std::int64_t>) {
            TEST_ASSERT_EQUAL_INT64(expected, value);
        } else if constexpr (std::is_same_v<T, double>) {
            TEST_ASSERT_DOUBLE_WITHIN(0.0001, expected, value);
        } else if constexpr (std::is_same_v<T, bool>) {
            TEST_ASSERT_EQUAL(expected, value);
        } else {
            TEST_FAIL_MESSAGE("Unhandled test case! Please update the tests");
        }

        TEST_PASS();
    }

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 30
    ));

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_config_get_object_sequence(
            1,
            { .bufs = keys.data(), .len = keys.size() },
            nullptr,
            gg::Object(expected)
        ),
        5
    ));

    GG_TEST_ASSERT_OK(gg_process_wait(pid));
};
}

extern "C" {
namespace tests {

    namespace {
        GG_TEST_DEFINE(get_config_top_level_key_okay) {
            std::span<gg::Buffer> keys {};

            std::string_view value = "Hello World!";
            std::array pairs = { gg::KV { "key", value } };
            gg::Map expected { pairs.data(), pairs.size() };

            gg::ipc::AllocatedObject obj;

            pid_t pid = fork();
            if (pid < 0) {
                TEST_IGNORE_MESSAGE("fork() failed.");
            }

            if (pid == 0) {
                auto &client = gg::ipc::Client::get();
                GG_TEST_ASSERT_OK(client.connect().value());

                try {
                    GG_TEST_ASSERT_OK(
                        client.get_config(keys, std::nullopt, obj).value()
                    );
                } catch (...) {
                    TEST_FAIL_MESSAGE("Exception caught from get_config");
                }

                gg::Object inner = obj.get();

                auto map = gg::get_if<gg::Map>(&inner);
                TEST_ASSERT_MESSAGE(
                    static_cast<bool>(map), "Result was not map"
                );

                TEST_ASSERT_EQUAL_size_t_MESSAGE(
                    1, map->size(), "Map size mismatch"
                );

                auto found = map->find("key");
                TEST_ASSERT_MESSAGE(found != map->cend(), "Key not found!");

                TEST_ASSERT_EQUAL_MESSAGE(
                    GG_TYPE_BUF, found->second->index(), "Value type mismatch"
                );

                auto buf = gg::get_if<gg::Buffer>(found->second);
                TEST_ASSERT_MESSAGE(
                    static_cast<bool>(buf), "Failed to get value"
                );
                TEST_ASSERT_EQUAL_size_t_MESSAGE(
                    value.size(), buf->size(), "Buffer size mismatch"
                );
                TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(
                    value.data(),
                    buf->data(),
                    value.size(),
                    "String contents mismatch"
                );

                TEST_PASS();
            }

            GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
                gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 30
            ));

            GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
                gg_test_config_get_object_sequence(
                    1,
                    GgBufList { .bufs = keys.data(), .len = keys.size() },
                    nullptr,
                    gg::Object(expected)
                ),
                5
            ));

            GG_TEST_ASSERT_OK(gg_process_wait(pid));
        }

        GG_TEST_DEFINE(get_config_str_nested_key_okay) {
            std::array keys = { gg::Buffer { "config" }, gg::Buffer { "key" } };

            std::string_view expected = "Hello World!";
            std::string value;
            try {
                value.reserve(16);
            } catch (std::bad_alloc &e) {
                TEST_FAIL_MESSAGE("Failed to allocate string");
            }

            pid_t pid = fork();
            if (pid < 0) {
                TEST_IGNORE_MESSAGE("fork() failed.");
            }

            if (pid == 0) {
                auto &client = gg::ipc::Client::get();
                GG_TEST_ASSERT_OK(client.connect().value());

                try {
                    GG_TEST_ASSERT_OK(
                        client.get_config(keys, std::nullopt, value).value()
                    );
                } catch (...) {
                    TEST_FAIL_MESSAGE("Exception caught from get_config");
                }

                TEST_ASSERT_EQUAL_size_t_MESSAGE(
                    expected.size(), value.size(), "Size mismatch!"
                );
                TEST_ASSERT_EQUAL_STRING_LEN(
                    expected.data(), value.data(), expected.size()
                );

                TEST_PASS();
            }

            GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
                gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 30
            ));

            GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
                gg_test_config_get_object_sequence(
                    1,
                    { .bufs = keys.data(), .len = keys.size() },
                    nullptr,
                    gg::Object(expected)
                ),
                5
            ));

            GG_TEST_ASSERT_OK(gg_process_wait(pid));
        }
    }

    GG_TEST_DEFINE(get_config_i64_okay) {
        test_config_overload<int64_t>(123456789LL);
    }

    GG_TEST_DEFINE(get_config_bool_true_okay) {
        test_config_overload(true);
    }

    GG_TEST_DEFINE(get_config_bool_false_okay) {
        test_config_overload(false);
    }

    GG_TEST_DEFINE(get_config_f64_okay) {
        test_config_overload(123.456);
    }
}
}
