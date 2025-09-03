#ifndef GGL_SDK_HPP
#define GGL_SDK_HPP

namespace ggl {
class Sdk {
private:
    Sdk() noexcept;

public:
    static Sdk &get() noexcept {
        static Sdk singleton {};
        return singleton;
    }
};

extern "C" {
inline Sdk::Sdk() noexcept {
    void ggl_sdk_init(void);
    ggl_sdk_init();
}
}

}

#endif
