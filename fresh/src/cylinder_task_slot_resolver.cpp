#include "pal4/cylinder_task_slot_resolver.h"

#include <array>

namespace pal4 {
namespace {

struct TaskSlot final {
    std::string_view name;
    std::uint32_t offset;
};

constexpr std::array kSlots{
    TaskSlot{"ShowobjTask", 20u},
    TaskSlot{"HideobjTask", 20u},
    TaskSlot{"ScaleTask", 24u},
    TaskSlot{"RotateTask", 28u},
    TaskSlot{"PositionTask", 32u},
    TaskSlot{"RevoluteTask", 36u},
    TaskSlot{"TrackTask", 40u},
    TaskSlot{"ChangeColorTask", 44u},
    TaskSlot{"RumpShahdowTask", 48u},
    TaskSlot{"MoveToTargetTask", 52u},
    TaskSlot{"MessageTask", 56u},
    TaskSlot{"SlowMotionTask", 56u},
};

}  // namespace

bool CylinderTaskSlotResolver::Resolve(
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
