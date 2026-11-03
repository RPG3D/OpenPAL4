#include "pal4/inventory_condition_record.h"

namespace pal4 {

std::int32_t InventoryConditionRecord::Serialize(
    const std::uint32_t record, const std::uint32_t stream,
    IInventoryConditionStreamRuntime& runtime) {
    runtime.WriteStreamWord(stream, record + 0x190u, 4u);
    return runtime.SerializeRecord(record, stream);
}

std::int32_t InventoryConditionRecord::Deserialize(
    const std::uint32_t record, const std::uint32_t stream,
    IInventoryConditionStreamRuntime& runtime) {
    runtime.ReadStreamWord(stream, record + 0x190u, 4u);
    return runtime.DeserializeRecord(record, stream);
}

bool InventoryConditionRecord::AnyActivePlayerHasItem(
    const std::uint32_t record, IInventoryConditionRuntime& runtime) {
    for (std::uint32_t index = 0; index < 4u; ++index) {
        const auto manager = runtime.InitializePlayerManager();
        const auto player = runtime.GetPlayerByIndex(manager, index);
        if (player != 0u && runtime.ReadWord(player + 0xb08u) != 0u &&
            runtime.InventoryContains(player + 0x920u,
                                      runtime.ReadWord(record + 0x190u))) {
            return true;
        }
    }
    return false;
}

bool InventoryConditionRecord::NoInactivePlayerHasItem(
    const std::uint32_t record, IInventoryConditionRuntime& runtime) {
    for (std::uint32_t index = 0; index < 4u; ++index) {
        const auto manager = runtime.InitializePlayerManager();
        const auto player = runtime.GetPlayerByIndex(manager, index);
        if (player != 0u && runtime.ReadWord(player + 0xb08u) == 0u &&
            runtime.InventoryContains(player + 0x920u,
                                      runtime.ReadWord(record + 0x190u))) {
            return false;
        }
    }
    return true;
}

}  // namespace pal4
