#pragma once

#include "pal4/property_window_lifecycle.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IPropertyWindowPanelEventsRuntime {
public:
    virtual ~IPropertyWindowPanelEventsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void UpdateCategoryPanel(PropertyWindowState& state) = 0;
    virtual void RemoveActiveMember(std::uint32_t callback) = 0;
    virtual void HideItemPanel(std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t HoverItem(std::uint32_t list) = 0;
    virtual void MoveHoverUp(std::uint32_t list) = 0;
    virtual void MoveHoverDown(std::uint32_t list) = 0;
    virtual void ShowItemSelection(
        std::uint32_t owner, const std::array<float, 2>& position) = 0;
};

class PropertyWindowPanelEvents final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x843a78u;

    [[nodiscard]] static bool HandleCategoryClick(
        PropertyWindowState& state, std::uint32_t event,
        IPropertyWindowPanelEventsRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructCallback(
        std::uint32_t object, IPropertyWindowPanelEventsRuntime& runtime);
    [[nodiscard]] static bool HandleItemPanelEvent(
        std::uint32_t callback, std::uint32_t event,
        IPropertyWindowPanelEventsRuntime& runtime);
};

}  // namespace pal4
