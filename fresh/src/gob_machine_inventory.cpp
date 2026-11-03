#include "pal4/gob_machine.h"
#include "pal4/gob_machine_inventory.h"

#include <string>

namespace pal4 {

namespace {

constexpr std::string_view kPropId = "PAL4_GameObject-machine-propid";
constexpr std::string_view kPropNum = "PAL4_GameObject-machine-propnum";

}  // namespace

std::uint32_t GobMachine::CheckAndConsumeInventoryConditions(
    const std::uint32_t object, IGobMachineInventoryRuntime& runtime) {
    const auto machine = runtime.MachineConfigNode(object);
    if (machine == 0u) return 0u;
    auto condition = runtime.FindCondition(machine, 0u);
    if (condition == 0u) return 1u;

    bool has_requirement = false;
    bool satisfied = true;
    for (; condition != 0u;
         condition = runtime.FindCondition(machine, condition)) {
        const auto id = runtime.ReadConditionInt(condition, kPropId, -1);
        const auto count = runtime.ReadConditionInt(condition, kPropNum, 0);
        if (id < 0) continue;
        has_requirement = true;
        const auto item = runtime.FindInventoryItem(id, 4u);
        if (item == 0u || runtime.ItemCount(item) < count) {
            satisfied = false;
            break;
        }
    }
    if (!has_requirement) return 1u;
    if (!runtime.CreateHintWindow()) return 0u;

    std::string hint;
    for (condition = runtime.FindCondition(machine, 0u);
         condition != 0u;
         condition = runtime.FindCondition(machine, condition)) {
        const auto id = runtime.ReadConditionInt(condition, kPropId, -1);
        const auto count = runtime.ReadConditionInt(condition, kPropNum, 0);
        if (id < 0) continue;
        if (!satisfied) {
            hint += runtime.FormatGameHint(0xf7u,
                                           runtime.PropertyName(id), count);
            continue;
        }
        const auto item = runtime.FindInventoryItem(id, 4u);
        if (item != 0u) {
            hint += runtime.FormatGameHint(
                0xf9u, runtime.InventoryItemName(item), count);
        } else {
            hint += runtime.FormatGameHint(0xfau, "", count);
        }
        runtime.RemoveInventoryItem(id, count);
    }
    runtime.ShowHint(hint, 0.5f, 0.15f, 1.0f, 0u, 1u);
    return satisfied ? 1u : 0u;
}

}  // namespace pal4
