#ifndef GGL_SDK_HPP
#define GGL_SDK_HPP

namespace ggl {
extern "C" {
inline void sdk_init() noexcept {
    void ggl_sdk_init(void) noexcept;
    ggl_sdk_init();
}
}
}

#endif
