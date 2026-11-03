#include "pal4/camera_property_cleanup.h"

namespace pal4 {

std::uint32_t CameraPropertyCleanup::Cleanup(CameraControllerState& state) {
    const auto result = runtime_.IsEmpty(state, 0x10u);
    if ((result & 0xffu) == 0u) {
        runtime_.ClearCollection(state, 0x10u);
        state.At(0x30u) = state.At(0x34u);
        return state.At(0x34u);
    }
    state.At(0x30u) = state.At(0x34u);
    return result;
}

}  // namespace pal4
