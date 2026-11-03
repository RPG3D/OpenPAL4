#pragma once

#include "pal4/property_window_lifecycle.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

struct PropertyHoveredItem {
    std::uint32_t address{};
    std::uint32_t definition{};
    std::uint32_t id{};
    std::uint32_t type{};
};

struct PropertySpecialEntry {
    std::string key;
    // IDA 4FFD10: Scene record+56 is EARTHBALL, tested as a signed value.
    std::int32_t earthball{};
};

class IPropertyWindowSelectionRuntime {
public:
    virtual ~IPropertyWindowSelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Dialog() = 0;
    virtual void HideAndCleanup(std::uint32_t dialog) = 0;
    [[nodiscard]] virtual std::optional<PropertyHoveredItem> HoveredItem(
        PropertyWindowState& state) = 0;
    [[nodiscard]] virtual bool CheckComponent(
        std::uint32_t definition) = 0;
    [[nodiscard]] virtual std::string ArenaObjectName() = 0;
    [[nodiscard]] virtual std::string ArenaSceneName() = 0;
    [[nodiscard]] virtual std::string BuildSpecialKey(
        std::string_view scene, std::string_view object) = 0;
    [[nodiscard]] virtual std::vector<PropertySpecialEntry>
        SpecialProperties() = 0;
    [[nodiscard]] virtual bool KeysEqual(
        std::string_view left, std::string_view right) = 0;
    [[nodiscard]] virtual std::string TriggerText(
        std::uint32_t trigger) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    virtual void CreateButton(std::uint32_t dialog,
                              std::string_view action,
                              std::uint32_t item,
                              std::string_view label,
                              bool enabled) = 0;
    virtual void ShowDialog(std::uint32_t dialog,
                            std::uint32_t event_data) = 0;
};

class PropertyWindowSelection final {
public:
    static void ShowChoices(PropertyWindowState& state,
                            std::uint32_t event_data,
                            IPropertyWindowSelectionRuntime& runtime);
};

}  // namespace pal4
