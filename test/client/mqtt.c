#include <errno.h>
#include <gg/arena.h>
#include <gg/base64.h>
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
#include <time.h>
#include <unistd.h>
#include <unity.h>
#include <stdatomic.h>
#include <stdlib.h>

#define GG_MODULE "test_mqtt"

static uint8_t too_large_payload[0x20000];

typedef struct {
    GgBuffer payload;
    GgBuffer payload_base64;
} PayloadPairs;

static PayloadPairs payloads[]
    = { { GG_STR("Hello world!"), GG_STR("SGVsbG8gd29ybGQh") },
        { GG_BUF(too_large_payload), /* don't need to encode */ { 0 } } };

GG_TEST_DEFINE(publish_to_iot_core_okay) {
    GgBuffer payload = payloads[0].payload;

    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        gg_sdk_init();
        GG_TEST_ASSERT_OK(ggipc_connect());
        GG_TEST_ASSERT_OK(
            ggipc_publish_to_iot_core(GG_STR("my/topic"), payload, 0)
        );
        TEST_PASS();
    }

    GgBuffer payload_base64 = payloads[0].payload_base64;

    GG_TEST_ASSERT_OK(gg_test_accept_client(1));

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 5
    ));

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_mqtt_publish_accepted_sequence(
            1, GG_STR("my/topic"), payload_base64, GG_STR("0")
        ),
        5
    ));

    GG_TEST_ASSERT_OK(gg_test_wait_for_client_disconnect(1));

    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}

GG_TEST_DEFINE(publish_to_iot_core_bad_alloc) {
    GgBuffer payload = payloads[1].payload;

    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        gg_sdk_init();
        GG_TEST_ASSERT_OK(ggipc_connect());
        TEST_ASSERT_EQUAL(
            GG_ERR_NOMEM,
            ggipc_publish_to_iot_core(GG_STR("my/topic"), payload, 0)
        );
        TEST_PASS();
    }

    GG_TEST_ASSERT_OK(gg_test_accept_client(1));

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 5
    ));

    GG_TEST_ASSERT_OK(gg_test_wait_for_client_disconnect(30));

    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}

GG_TEST_DEFINE(publish_to_iot_core_b64_okay) {
    GgBuffer payload_base64 = payloads[0].payload_base64;

    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        gg_sdk_init();
        GG_TEST_ASSERT_OK(ggipc_connect());
        GG_TEST_ASSERT_OK(
            ggipc_publish_to_iot_core_b64(GG_STR("my/topic"), payload_base64, 0)
        );
        TEST_PASS();
    }

    GG_TEST_ASSERT_OK(gg_test_accept_client(1));

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 5
    ));

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_mqtt_publish_accepted_sequence(
            1, GG_STR("my/topic"), payload_base64, GG_STR("0")
        ),
        5
    ));

    GG_TEST_ASSERT_OK(gg_test_wait_for_client_disconnect(1));

    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}

GG_TEST_DEFINE(publish_to_iot_core_rejected) {
    GgBuffer payload = payloads[0].payload;

    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        gg_sdk_init();
        GG_TEST_ASSERT_OK(ggipc_connect());
        GG_TEST_ASSERT_BAD(
            ggipc_publish_to_iot_core(GG_STR("my/topic"), payload, 0)
        );
        TEST_PASS();
    }

    GgBuffer payload_base64 = payloads[0].payload_base64;

    GG_TEST_ASSERT_OK(gg_test_accept_client(1));

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 5
    ));

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_mqtt_publish_error_sequence(
            1, GG_STR("my/topic"), payload_base64, GG_STR("0")
        ),
        5
    ));

    GG_TEST_ASSERT_OK(gg_test_wait_for_client_disconnect(1));

    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}

GG_TEST_DEFINE(publish_to_iot_core_invalid_qos) {
    GgBuffer payload = payloads[0].payload;

    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        gg_sdk_init();
        GG_TEST_ASSERT_OK(ggipc_connect());
        GG_TEST_ASSERT_BAD(
            ggipc_publish_to_iot_core(GG_STR("my/topic"), payload, 10)
        );
        TEST_PASS();
    }

    GG_TEST_ASSERT_OK(gg_test_accept_client(1));

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 5
    ));

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_mqtt_publish_error_sequence(
            1,
            GG_STR("my/topic"),
            payloads[0].payload_base64,
            GG_BUF((uint8_t[]) { '0' + 10 })
        ),
        5
    ));

    GG_TEST_ASSERT_OK(gg_test_wait_for_client_disconnect(1));

    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}

typedef struct {
    pthread_mutex_t mut;
    _Atomic size_t calls_remaining;
    pthread_cond_t cond;
    GgIpcSubscriptionHandle handle;
} SubscribeOkayContext;

static const size_t EXPECTED_TIMES_CALLED = 3;
static SubscribeOkayContext subscribe_okay_context
    = { .calls_remaining = EXPECTED_TIMES_CALLED,
        .mut = PTHREAD_MUTEX_INITIALIZER,
        .cond = PTHREAD_COND_INITIALIZER,
        .handle = { 0 } };

static void subscribe_to_iot_core_okay_subscription_response(
    void *ctx, GgBuffer topic, GgBuffer payload, GgIpcSubscriptionHandle handle
) {
    SubscribeOkayContext *context = ctx;
    TEST_ASSERT_EQUAL_PTR(&subscribe_okay_context, context);
    GG_TEST_ASSERT_BUF_EQUAL_STR(GG_STR("my/topic"), topic);

    GG_TEST_ASSERT_BUF_EQUAL(payloads[0].payload, payload);

    TEST_ASSERT_EQUAL_UINT32(context->handle.val, handle.val);

    size_t calls_remaining_prev
        = atomic_fetch_sub(&context->calls_remaining, 1);
    if ((calls_remaining_prev - 1 == 0)
        || (calls_remaining_prev > EXPECTED_TIMES_CALLED)) {
        pthread_cond_signal(&context->cond);
    }
}

GG_TEST_DEFINE(subscribe_to_iot_core_okay) {
    pid_t pid = fork();
    TEST_ASSERT_TRUE_MESSAGE(pid >= 0, "fork failed");

    if (pid == 0) {
        gg_sdk_init();
        GG_TEST_ASSERT_OK(ggipc_connect());
        GG_TEST_ASSERT_OK(ggipc_subscribe_to_iot_core(
            GG_STR("my/topic"),
            0,
            subscribe_to_iot_core_okay_subscription_response,
            &subscribe_okay_context,
            &subscribe_okay_context.handle
        ));

        int pthread_ret = 0;
        size_t calls_remaining;
        struct timespec wait_until;
        clock_gettime(CLOCK_REALTIME, &wait_until);
        wait_until.tv_sec += 1;

        pthread_mutex_lock(&subscribe_okay_context.mut);
        while (((calls_remaining
                 = atomic_load(&subscribe_okay_context.calls_remaining))
                != 0)
               && (calls_remaining <= EXPECTED_TIMES_CALLED)) {
            pthread_ret = pthread_cond_timedwait(
                &subscribe_okay_context.cond,
                &subscribe_okay_context.mut,
                &wait_until
            );
            if (pthread_ret != 0) {
                GG_LOGW("pthread_cond_timedwait timed out");
                calls_remaining
                    = atomic_load(&subscribe_okay_context.calls_remaining);
                break;
            }
        }
        pthread_mutex_unlock(&subscribe_okay_context.mut);

        TEST_ASSERT_EQUAL_size_t_MESSAGE(
            EXPECTED_TIMES_CALLED,
            EXPECTED_TIMES_CALLED - calls_remaining,
            "Subscription not called the expected number of times."
        );
        TEST_ASSERT_EQUAL_MESSAGE(
            0, pthread_ret, "Timed out waiting for subscription response(s)"
        );
        TEST_PASS();
    }

    GG_TEST_ASSERT_OK(gg_test_accept_client(1));

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_connect_accepted_sequence(gg_test_get_auth_token()), 5
    ));

    GG_TEST_ASSERT_OK(gg_test_expect_packet_sequence(
        gg_test_mqtt_subscribe_accepted_sequence(
            1,
            GG_STR("my/topic"),
            payloads[0].payload_base64,
            GG_STR("0"),
            EXPECTED_TIMES_CALLED
        ),
        30
    ));

    GG_TEST_ASSERT_OK(gg_test_wait_for_client_disconnect(5));

    GG_TEST_ASSERT_OK(gg_process_wait(pid));
}
