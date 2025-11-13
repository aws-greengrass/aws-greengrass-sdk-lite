#include <gg/buffer.hpp>
#include <gg/error.hpp>
#include <gg/ipc/client.hpp>
#include <gg/ipc/client_c_api.hpp>
#include <gg/ipc/subscription.hpp>
#include <gg/types.hpp>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <source_location>
#include <string_view>
#include <system_error>

namespace gg::ipc {
extern "C" {
namespace {
    void subscribe_to_iot_core_callback(
        void *ctx,
        GgBuffer topic,
        GgBuffer payload,
        GgIpcSubscriptionHandle handle
    ) noexcept try {
        Subscription locked { handle };
        std::invoke(
            *static_cast<IotTopicCallback *>(ctx),
            std::string_view { reinterpret_cast<char *>(topic.data),
                               topic.len },
            payload,
            locked
        );
        (void) locked.release();
    } catch (const std::exception &e) {
        std::cerr << "Exception caught in "
                  << std::source_location {}.function_name() << '\n'
                  << e.what() << '\n';
    } catch (...) {
        std::cerr << "Exception caught in "
                  << std::source_location {}.function_name() << '\n';
    }
}
}

// singleton interface class.
// NOLINTBEGIN(readability-convert-member-functions-to-static)

std::error_code Client::subscribe_to_iot_core(
    std::string_view topic_filter,
    std::uint8_t qos,
    IotTopicCallback &callback,
    Subscription *handle
) noexcept {
    GgIpcSubscriptionHandle raw_handle;
    GgError ret = ggipc_subscribe_to_iot_core(
        gg::Buffer { topic_filter },
        qos,
        subscribe_to_iot_core_callback,
        &callback,
        (handle != nullptr) ? &raw_handle : nullptr
    );
    if ((handle != nullptr) && (ret == GG_ERR_OK)) {
        handle->reset(raw_handle);
    }
    return ret;
}

// NOLINTEND(readability-convert-member-functions-to-static)

}
