#include "pal4/sequence_anim_task_slot_resolver.h"

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
    TaskSlot{"PositionTask", 24u},
    TaskSlot{"RevoluteTask", 28u},
    TaskSlot{"TrackTask", 32u},
    TaskSlot{"ChangeColorTask", 36u},
    TaskSlot{"ScaleTask", 40u},
    TaskSlot{"RotateTask", 44u},
    TaskSlot{"MessageTask", 48u},
    TaskSlot{"SlowMotionTask", 48u}};

}  // namespace

bool SequenceAnimTaskSlotResolver::Resolve(
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
