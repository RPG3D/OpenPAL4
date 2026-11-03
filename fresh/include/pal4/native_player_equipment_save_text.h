#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

class NativeInventorySession;

class NativePlayerEquipmentSaveText final {
public:
    // IDA 0x42e6d0: the ID identifies a runtime inventory entry.
    [[nodiscard]] static std::string Format(
        std::int32_t instance_id, std::optional<std::string_view> name);
    [[nodiscard]] static std::string FormatFromInventory(
        NativeInventorySession& inventory, std::int32_t instance_id);
};

}  // namespace pal4
