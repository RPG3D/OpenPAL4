#include "pal4/camera_allocation.h"

namespace pal4 {

std::optional<std::uint32_t> CameraAllocation::Release(
    std::uint32_t& camera_data) {
    if (camera_data == 0u) return std::nullopt;
    const auto allocation = camera_data;
    runtime_.DestroyDevice(allocation);
    runtime_.DestroyData(allocation);
    const auto result = runtime_.Free(allocation);
    camera_data = 0u;
    return result;
}

}  // namespace pal4
