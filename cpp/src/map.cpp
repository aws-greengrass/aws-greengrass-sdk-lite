#include "ggl/map.hpp"
#include "ggl/object.hpp"
#include "ggl/types.hpp"

namespace ggl {

Object *KV::value() noexcept {
    return static_cast<Object *>(ggl_kv_val(this));
}

}
