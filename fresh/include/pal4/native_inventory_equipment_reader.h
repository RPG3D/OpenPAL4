#pragma once

#include <cstdint>

namespace pal4 {
class NativeInventorySession;
class NativeGameSaveArchiveReader;

// Restores the equipment member at Inventory+0x7c from the current stream
// position. The inventory owns all restored records, text and slot buffers.
class NativeInventoryEquipmentReader final {
public:
    [[nodiscard]] static std::uint8_t Deserialize(
        NativeInventorySession& inventory, NativeGameSaveArchiveReader& archive);
};
}  // namespace pal4
