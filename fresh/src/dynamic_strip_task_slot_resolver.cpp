#include "pal4/dynamic_strip_task_slot_resolver.h"

#include <array>

namespace pal4 {
namespace {

struct TaskSlot final {
    std::string_view name;
    std::uint32_t offset;
};

constexpr std::array kSlots{
    TaskSlot{"ShowobjTask", 40u},
    TaskSlot{"HideobjTask", 40u},
    TaskSlot{"ScaleTask", 20u},
    TaskSlot{"RotateTask", 24u},
    TaskSlot{"PositionTask", 28u},
    TaskSlot{"RevoluteTask", 32u},
    TaskSlot{"TrackTask", 36u},
    TaskSlot{"ChangeColorTask", 44u},
    TaskSlot{"MessageTask", 48u},
    TaskSlot{"SlowMotionTask", 48u},
};

}  // namespace

bool DynamicStripTaskSlotResolver::Resolve(
    const std::uint32_t object, const std::string_view task_name,
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
