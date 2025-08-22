#ifndef GGL_IPC_CLIENT_HPP
#define GGL_IPC_CLIENT_HPP

#include "ggl/error.hpp"
#include <string_view>

namespace ggl {
class Map;
class List;
class Buffer;
class Object;
}

namespace ggl::ipc {
GglError connect() noexcept;
GglError connect(
    std::string_view socket_path, std::string_view auth_token
) noexcept;

GglError publish_to_iot_core(
    std::string_view topic, std::string_view payload
) noexcept;
GglError publish_to_iot_core(
    std::string_view topic, const Map &payload
) noexcept;

}

#endif
