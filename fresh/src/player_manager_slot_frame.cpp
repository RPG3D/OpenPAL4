#include "pal4/player_manager_slot_frame.h"

namespace pal4 {

std::uint32_t PlayerManagerSlotFrameRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void PlayerManagerSlotFrameRuntimeStub::UpdatePosition(
    float, std::uint32_t) {}
std::uint32_t PlayerManagerSlotFrameRuntimeStub::UpdateAnimationList(
    std::uint32_t, float) { return 0u; }

std::uint32_t PlayerManagerSlotFrame::Update(
    std::uint32_t object, float elapsed, std::uint32_t mode,
    IPlayerManagerSlotFrameRuntime& runtime) {
    if (runtime.ReadDword(object + 0x1f0u) != 0u)
        runtime.UpdatePosition(elapsed, mode);
    return runtime.UpdateAnimationList(object + 0xaa8u, elapsed);
}

}  // namespace pal4
