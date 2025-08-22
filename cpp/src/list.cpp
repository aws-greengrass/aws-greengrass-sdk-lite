#include "ggl/list.hpp"
#include "ggl/object.hpp"

namespace ggl {

List::reference List::operator[](size_type pos) const noexcept {
    return data()[pos];
}

List::pointer List::data() const noexcept {
    return static_cast<Object *>(items);
}

}
