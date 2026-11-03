#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IPropertyWindowSpecialItemRuntime {
public:
    virtual ~IPropertyWindowSpecialItemRuntime() = default;
    virtual void ToggleModel(std::uint32_t frame, std::uint32_t context,
                             std::uint32_t visible) = 0;
    virtual void UpdateQuestModel(std::uint32_t frame,
                                  std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    virtual void RemoveItem(std::uint32_t inventory,
                            std::uint32_t item_id,
                            std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    [[nodiscard]] virtual std::uint8_t ProcessEvents(
        std::uint32_t event_table, std::uint32_t event_id,
        const std::array<std::uint32_t, 2>& payload) = 0;
};

class PropertyWindowSpecialItem final {
public:
    static constexpr std::uint32_t kEventId = 0x8a7898u;
    [[nodiscard]] static std::uint8_t UseMagicItem(
        std::uint32_t frame, std::uint32_t context,
        IPropertyWindowSpecialItemRuntime& runtime);
};

}  // namespace pal4
