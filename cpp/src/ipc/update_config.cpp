#include "ggl/ipc/client.hpp"
#include "ggl/ipc/client_c_api.hpp"
#include <chrono>
#include <ctime>
#include <ratio>
#include <system_error>

namespace ggl::ipc {
// singleton interface class.
// NOLINTBEGIN(readability-convert-member-functions-to-static)

std::error_code Client::update_config(
    std::span<const Buffer> key_path,
    std::optional<std::chrono::system_clock::time_point> timestamp,
    const Object &value
) noexcept {
    std::timespec ts {};
    if (timestamp.has_value()) {
        using namespace std::chrono;
        ts.tv_sec
            = duration_cast<seconds>(timestamp->time_since_epoch()).count();

        ts.tv_nsec = (duration_cast<nanoseconds>(timestamp->time_since_epoch())
                      % std::nano::den)
                         .count();
    }

    return ggipc_update_config(
        { .bufs = const_cast<Buffer *>(key_path.data()),
          .len = key_path.size() },
        &ts,
        value
    );
}

// NOLINTEND(readability-convert-member-functions-to-static)

}
