#include "pal4/base_object_static_accessors.h"

namespace pal4 {

void BaseObjectStaticAccessors::SetDefaultVtable(BaseObjectState& state) {
    state.Write(0u, 0x8425a0u);
}

const char* BaseObjectStaticAccessors::HoldString() { return "hold"; }

const char* BaseObjectStaticAccessors::RpeString() { return "rpe"; }

}  // namespace pal4
