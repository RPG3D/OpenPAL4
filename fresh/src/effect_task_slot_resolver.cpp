#include "pal4/effect_task_slot_resolver.h"

#include <array>

namespace pal4 {
namespace {

struct TaskSlot {
    std::string_view name;
    std::uint32_t offset;
};

constexpr std::array kSlots{
    TaskSlot{"ShowobjTask", 0x14u},
    TaskSlot{"HideobjTask", 0x14u},
    TaskSlot{"PositionTask", 0x18u},
    TaskSlot{"RevoluteTask", 0x1cu},
    TaskSlot{"TrackTask", 0x20u},
    TaskSlot{"ChangeColorTask", 0x24u},
    TaskSlot{"ScaleTask", 0x28u},
    TaskSlot{"RotateTask", 0x2cu},
    TaskSlot{"RumpShahdowTask", 0x30u},
    TaskSlot{"MessageTask", 0x34u},
    TaskSlot{"SlowMotionTask", 0x34u},
};

}  // namespace

bool EffectTaskSlotResolver::Resolve(const std::uint32_t object,
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
