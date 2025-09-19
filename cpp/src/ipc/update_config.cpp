#include "ggl/ipc/client.hpp"
#include "ggl/ipc/client_c_api.hpp"
#include <chrono>
#include <ctime>
#include <system_error>

namespace ggl::ipc {
// singleton interface class.
// NOLINTBEGIN(readability-convert-member-functions-to-static)

std::error_code Client::update_config(
    std::span<const Buffer> key_path,
    const Object &value,
    std::chrono::system_clock::time_point timestamp
) noexcept {
    using namespace std::chrono;
    auto rel_time = timestamp.time_since_epoch();
    auto sec = duration_cast<seconds>(rel_time);
    auto nsec = duration_cast<nanoseconds>(rel_time - sec);
    std::timespec ts
        = { .tv_sec = static_cast<std::time_t>(sec.count()),
            .tv_nsec = static_cast<decltype(ts.tv_nsec)>(nsec.count()) };

    return ggipc_update_config(
        { .bufs = const_cast<Buffer *>(key_path.data()),
          .len = key_path.size() },
        &ts,
        value
    );
}

// NOLINTEND(readability-convert-member-functions-to-static)

}
