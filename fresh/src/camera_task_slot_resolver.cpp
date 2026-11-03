#include "pal4/camera_task_slot_resolver.h"

#include <array>

namespace pal4 {
namespace {

struct TaskSlot final {
    std::string_view name;
    std::uint32_t offset;
};

constexpr std::array kSlots{
    TaskSlot{"StartCameraTask", 24u},
    TaskSlot{"StopCameraTask", 24u},
    TaskSlot{"TrackTask", 20u},
    TaskSlot{"RevoluteTask", 28u},
    TaskSlot{"PositionTask", 32u},
    TaskSlot{"WobbleTask", 32u},
    TaskSlot{"RotateTask", 36u},
    TaskSlot{"MessageTask", 40u},
    TaskSlot{"SlowMotionTask", 40u},
};

}  // namespace

bool CameraTaskSlotResolver::Resolve(const std::uint32_t object,
                                     const std::string_view task_name,
                                     std::uint32_t& output) {
    for (const auto& slot : kSlots) {
        if (task_name == slot.name) {
            output = object == 0u ? 0u : object + slot.offset;
            return output != 0u;
        }
    }
    output = 0u;
    return false;
}

}  // namespace pal4
