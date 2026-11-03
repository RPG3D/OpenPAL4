#include "pal4/camera_system_cleanup.h"
#include "pal4/camera_allocation.h"

namespace pal4 {

std::uint32_t CameraSystemCleanup::Cleanup(CameraControllerState& state) {
    for (std::uint32_t offset = 0x34u; offset <= 0x40u; offset += 4u) {
        const auto address = state.At(offset);
        if (address == 0u) continue;
        static_cast<void>(allocation_.Release(runtime_.ObjectAt(address).camera));
        const auto current = state.At(offset);
        if (current != 0u) {
            static_cast<void>(object_.Destroy(runtime_.ObjectAt(current)));
            runtime_.FreeObject(current);
        }
        state.At(offset) = 0u;
    }
    static_cast<void>(properties_.Cleanup(state));
    const auto result = CameraController::ClearDynamicCollection(state);
    state.At(0x30u) = 0u;
    state.At(0xbcu) = 0u;
    return result;
}

}  // namespace pal4
