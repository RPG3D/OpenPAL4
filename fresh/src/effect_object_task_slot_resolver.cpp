#include "pal4/effect_object_task_slot_resolver.h"

#include <array>

namespace pal4 {
namespace {

struct TaskSlot final {
    std::string_view name;
    std::uint32_t offset;
};

constexpr std::array kSlots{
    TaskSlot{"ScaleTask", 20u},
    TaskSlot{"RotateTask", 24u},
    TaskSlot{"PositionTask", 28u},
    TaskSlot{"RevoluteTask", 32u},
    TaskSlot{"PlayEffectTask", 36u},
    TaskSlot{"PauseEffectTask", 36u},
    TaskSlot{"StopEffectTask", 36u},
    TaskSlot{"RewindEffectTask", 36u},
    TaskSlot{"TrackTask", 40u},
    TaskSlot{"MessageTask", 44u},
    TaskSlot{"SlowMotionTask", 44u},
    TaskSlot{"MoveToTargetTask", 48u}};

}  // namespace

bool EffectObjectTaskSlotResolver::Resolve(
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
