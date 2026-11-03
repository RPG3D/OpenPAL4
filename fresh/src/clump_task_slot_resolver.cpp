#include "pal4/clump_task_slot_resolver.h"

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
    TaskSlot{"ShowobjTask", 40u},
    TaskSlot{"HideobjTask", 40u},
    TaskSlot{"UVRotateTask", 44u},
    TaskSlot{"UVTranslateTask", 48u},
    TaskSlot{"RevoluteTask", 32u},
    TaskSlot{"ChangeColorTask", 36u},
    TaskSlot{"TrackTask", 52u},
    TaskSlot{"MoveToTargetTask", 56u},
    TaskSlot{"RumpShahdowTask", 60u},
    TaskSlot{"MessageTask", 64u},
    TaskSlot{"SlowMotionTask", 64u},
};

}  // namespace

bool ClumpTaskSlotResolver::Resolve(const std::uint32_t object,
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
