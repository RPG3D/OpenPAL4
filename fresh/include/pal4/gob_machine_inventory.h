#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGobMachineInventoryRuntime {
public:
    virtual ~IGobMachineInventoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t MachineConfigNode(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t FindCondition(
        std::uint32_t machine, std::uint32_t previous) = 0;
    [[nodiscard]] virtual std::int32_t ReadConditionInt(
        std::uint32_t condition, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryItem(
        std::int32_t id, std::uint32_t type) = 0;
    [[nodiscard]] virtual std::int32_t ItemCount(
        std::uint32_t item) = 0;
    [[nodiscard]] virtual bool CreateHintWindow() = 0;
    [[nodiscard]] virtual std::string PropertyName(
        std::int32_t id) = 0;
    [[nodiscard]] virtual std::string InventoryItemName(
        std::uint32_t item) = 0;
    [[nodiscard]] virtual std::string FormatGameHint(
        std::uint32_t text_id, std::string_view item_name,
        std::int32_t count) = 0;
    virtual void RemoveInventoryItem(std::int32_t id,
                                     std::int32_t count) = 0;
    virtual void ShowHint(std::string_view message, float x, float y,
                          float scale, std::uint32_t first,
                          std::uint32_t second) = 0;
};

}  // namespace pal4
