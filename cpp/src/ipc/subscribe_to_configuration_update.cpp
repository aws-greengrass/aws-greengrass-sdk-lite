#include <gg/buffer.hpp>
#include <gg/error.hpp>
#include <gg/ipc/client.hpp>
#include <gg/ipc/client_c_api.hpp>
#include <gg/ipc/subscription.hpp>
#include <gg/types.hpp>
#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <source_location>
#include <span>
#include <string_view>
#include <system_error>

namespace gg::ipc {
extern "C" {
namespace {
    void subscribe_to_configuration_update_callback(
        void *ctx,
        GgBuffer component_name,
        GgList key_path,
        GgIpcSubscriptionHandle handle
    ) noexcept try {
        Subscription locked { handle };
        std::invoke(
            *static_cast<ConfigurationUpdateCallback *>(ctx),
            std::string_view { reinterpret_cast<char *>(component_name.data),
                               component_name.len },
            key_path,
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

std::error_code Client::subscribe_to_configuration_update(
    std::span<const Buffer> key_path,
    std::optional<std::string_view> component_name,
    ConfigurationUpdateCallback &callback,
    Subscription *handle
) noexcept {
    gg::Buffer component_name_buf {
        component_name.value_or(std::string_view {})
    };

    GgIpcSubscriptionHandle raw_handle;
    GgError ret = ggipc_subscribe_to_configuration_update(
        component_name.has_value() ? &component_name_buf : nullptr,
        GgBufList { .bufs = const_cast<gg::Buffer *>(key_path.data()),
                    .len = key_path.size() },
        subscribe_to_configuration_update_callback,
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
