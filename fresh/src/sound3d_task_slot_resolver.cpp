#include "pal4/sound3d_task_slot_resolver.h"

#include <array>

namespace pal4 {
namespace {

struct TaskSlot final {
    std::string_view name;
    std::uint32_t offset;
};
constexpr std::array kSlots{
    TaskSlot{"PlaySound3DTask", 28u},
    TaskSlot{"StopSound3DTask", 28u},
    TaskSlot{"RevoluteTask", 24u},
    TaskSlot{"PositionTask", 20u},
    TaskSlot{"TrackTask", 32u},
    TaskSlot{"MessageTask", 36u},
    TaskSlot{"SlowMotionTask", 36u}};

}  // namespace

bool Sound3DTaskSlotResolver::Resolve(
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
